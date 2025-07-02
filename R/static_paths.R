#' Create a staticPath object
#'
#' The \code{staticPath} function creates a \code{staticPath} object. Note that
#' if any of the arguments (other than \code{path}) are \code{NULL}, then that
#' means that for this particular static path, it should inherit the behavior
#' from the staticPathOptions set for the application as a whole.
#'
#' The \code{excludeStaticPath} function tells the application to ignore a
#' particular path for static serving. This is useful when you want to include a
#' path for static serving (like \code{"/"}) but then exclude a subdirectory of
#' it (like \code{"/dynamic"}) so that the subdirectory will always be passed to
#' the R code for handling requests. \code{excludeStaticPath} can be used not
#' only for directories; it can also exclude specific files.
#'
#' @param path The local path.
#' @inheritParams staticPathOptions
#'
#' @seealso \code{\link{staticPathOptions}}.
#'
#' @export
staticPath <- function(
  path,
  indexhtml = NULL,
  fallthrough = NULL,
  html_charset = NULL,
  headers = NULL,
  validation = NULL
) {
  if (!is.character(path) || length(path) != 1 || path == "") {
    stop("`path` must be a non-empty string.")
  }

  path <- normalizePath(path, winslash = "/", mustWork = TRUE)
  path <- enc2utf8(path)

  structure(
    list(
      path = path,
      options = normalizeStaticPathOptions(staticPathOptions(
        indexhtml = indexhtml,
        fallthrough = fallthrough,
        html_charset = html_charset,
        headers = headers,
        validation = validation,
        exclude = FALSE
      ))
    ),
    class = "staticPath"
  )
}

#' @rdname staticPath
#' @export
excludeStaticPath <- function() {
  structure(
    list(
      path = "",
      options = staticPathOptions(
        indexhtml = NULL,
        fallthrough = NULL,
        html_charset = NULL,
        headers = NULL,
        validation = NULL,
        exclude = TRUE
      )
    ),
    class = "staticPath"
  )
}

as.staticPath <- function(path) {
  UseMethod("as.staticPath", path)
}

as.staticPath.staticPath <- function(path) {
  path
}

as.staticPath.character <- function(path) {
  staticPath(path)
}

as.staticPath.default <- function(path) {
  stop(
    "Cannot convert object of class ",
    class(path),
    " to a staticPath object."
  )
}

#' @export
print.staticPath <- function(x, ...) {
  cat(format(x, ...), sep = "\n")
  invisible(x)
}

#' @export
format.staticPath <- function(x, ...) {
  ret <- paste0(
    "<staticPath>\n",
    "  Local path:        ",
    x$path,
    "\n",
    format_opts(x$options)
  )
}

#' Create options for static paths
#'
#'
#' @param indexhtml If an index.html file is present, should it be served up
#'   when the client requests the static path or any subdirectory?
#' @param fallthrough With the default value, \code{FALSE}, if a request is made
#'   for a file that doesn't exist, then httpuv will immediately send a 404
#'   response from the background I/O thread, without needing to call back into
#'   the main R thread. This offers the best performance. If the value is
#'   \code{TRUE}, then instead of sending a 404 response, httpuv will call the
#'   application's \code{call} function, and allow it to handle the request.
#' @param html_charset When HTML files are served, the value that will be
#'   provided for \code{charset} in the Content-Type header. For example, with
#'   the default value, \code{"utf-8"}, the header is \code{Content-Type:
#'   text/html; charset=utf-8}. If \code{""} is used, then no \code{charset}
#'   will be added in the Content-Type header.
#' @param headers Additional headers and values that will be included in the
#'   response.
#' @param validation An optional validation pattern. Presently, the only type of
#'   validation supported is an exact string match of a header. For example, if
#'   \code{validation} is \code{'"abc" = "xyz"'}, then HTTP requests must have a
#'   header named \code{abc} (case-insensitive) with the value \code{xyz}
#'   (case-sensitive). If a request does not have a matching header, than httpuv
#'   will give a 403 Forbidden response. If the \code{character(0)} (the
#'   default), then no validation check will be performed.
#' @param exclude Should this path be excluded from static serving? (This is
#'   only to be used internally, for \code{\link{excludeStaticPath}}.)
#'
#' @export
staticPathOptions <- function(
  indexhtml = TRUE,
  fallthrough = FALSE,
  html_charset = "utf-8",
  headers = list(),
  validation = character(0),
  exclude = FALSE
) {
  res <- structure(
    list(
      indexhtml = indexhtml,
      fallthrough = fallthrough,
      html_charset = html_charset,
      headers = headers,
      validation = validation,
      exclude = exclude
    ),
    class = "staticPathOptions"
  )

  normalizeStaticPathOptions(res)
}

#' @export
print.staticPathOptions <- function(x, ...) {
  cat(format(x, ...), sep = "\n")
  invisible(x)
}


#' @export
format.staticPathOptions <- function(x, ...) {
  paste0(
    "<staticPathOptions>\n",
    format_opts(x, format_empty = "<none>")
  )
}

format_opts <- function(x, format_empty = "<inherit>") {
  format_option <- function(opt) {
    if (is.null(opt) || length(opt) == 0) {
      format_empty
    } else if (!is.null(names(opt))) {
      # Named character vector
      lines <- mapply(
        function(name, value) paste0('    "', name, '" = "', value, '"'),
        names(opt),
        opt,
        SIMPLIFY = FALSE,
        USE.NAMES = FALSE
      )

      lines <- paste(as.character(lines), collapse = "\n")
      lines <- paste0("\n", lines)
      lines
    } else {
      paste(as.character(opt), collapse = " ")
    }
  }
  ret <- paste0(
    "  Use index.html:    ",
    format_option(x$indexhtml),
    "\n",
    "  Fallthrough to R:  ",
    format_option(x$fallthrough),
    "\n",
    "  HTML charset:      ",
    format_option(x$html_charset),
    "\n",
    "  Extra headers:     ",
    format_option(x$headers),
    "\n",
    "  Validation params: ",
    format_option(x$validation),
    "\n",
    "  Exclude path:      ",
    format_option(x$exclude),
    "\n"
  )
}


# This function always returns a named list of staticPath objects. The names
# will all start with "/". The input can be a named character vector or a
# named list containing a mix of strings and staticPath objects. This function
# is idempotent.
normalizeStaticPaths <- function(paths) {
  if (is.null(paths) || length(paths) == 0) {
    return(list())
  }

  if (any_unnamed(paths)) {
    stop(
      "paths must be a named character vector, a named list containing strings and staticPath objects, or NULL."
    )
  }

  if (!is.character(paths) && !is.list(paths)) {
    stop(
      "paths must be a named character vector, a named list containing strings and staticPath objects, or NULL."
    )
  }

  # Convert to list of staticPath objects. Need this verbose wrapping of
  # as.staticPath because of S3 dispatch for non-registered methods.
  paths <- lapply(paths, function(path) as.staticPath(path))

  # Make sure URL paths have a leading '/' and no trailing '/'.
  names(paths) <- vapply(
    names(paths),
    function(path) {
      path <- enc2utf8(path)

      if (path == "") {
        stop("All paths must be non-empty strings.")
      }
      # Ensure there's a leading / for every path
      if (substr(path, 1, 1) != "/") {
        path <- paste0("/", path)
      }
      # Strip trailing slashes, except when the path is just "/".
      if (path != "/") {
        path <- sub("/+$", "", path)
      }

      path
    },
    ""
  )

  paths
}

# Takes a staticPathOptions object and modifies it so that the resulting
# object is easier to work with on the C++ side. The resulting object is not
# meant to be modified on the R side. This function is idempotent; if the
# object has already been normalized, it will not be modified. For each entry,
# a NULL means to inherit.
normalizeStaticPathOptions <- function(opts) {
  if (isTRUE(attr(opts, "normalized", exact = TRUE))) {
    return(opts)
  }

  # html_charset can accept "" or character(0). But on the C++ side, we want
  # "".
  if (!is.null(opts$html_charset)) {
    if (length(opts$html_charset) == 0) {
      opts$html_charset <- ""
    }
  }

  if (!is.null(opts$exclude)) {
    if (!is.logical(opts$exclude) || length(opts$exclude) != 1) {
      stop("`exclude` option must be TRUE or FALSE.")
    }
  }

  # Can be a named list of strings, or a named character vector. On the C++
  # side, we want a named character vector.
  if (is.list(opts$headers)) {
    # Convert list to named character vector
    opts$headers <- unlist(opts$headers, recursive = FALSE)
    # Special case: if opts$headers was an empty list before unlist(), it is
    # now NULL. Replace it with an empty named character vector.
    if (length(opts$headers) == 0) {
      opts$headers <- c(a = "a")[0]
    }

    if (!is.character(opts$headers) || any_unnamed(opts$headers)) {
      stop("`headers` option must be a named list or character vector.")
    }
  }

  if (!is.null(opts$validation)) {
    if (!is.character(opts$validation) || length(opts$validation) > 1) {
      stop(
        "`validation` option must be a character vector with zero or one element."
      )
    }

    # Both "" and character(0) result in character(0). Length-1 strings other
    # than "" will be parsed.
    if (length(opts$validation) == 1) {
      if (opts$validation == "") {
        opts$validation <- character(0)
      } else {
        fail <- FALSE
        tryCatch(
          p <- parse(text = opts$validation)[[1]],
          error = function(e) fail <<- TRUE
        )
        if (!fail) {
          if (
            length(p) != 3 ||
              p[[1]] != as.symbol("==") ||
              !is.character(p[[2]]) ||
              length(p[[2]]) != 1 ||
              !is.character(p[[3]]) ||
              length(p[[3]]) != 1
          ) {
            fail <- TRUE
          }
        }
        if (fail) {
          stop(
            "`validation` must be a string of the form: '\"xxx\" == \"yyy\"'"
          )
        }

        # Turn it into a char vector for easier processing in C++
        opts$validation <- as.character(p)
      }
    }
  }

  attr(opts, "normalized") <- TRUE
  opts
}
