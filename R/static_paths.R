#' Create a staticPath object
#'
#' This function creates a \code{staticPath} object.
#'
#' @param path The local path.
#' @param indexhtml If an index.html file is present, should it be served up when
#'   the client requests \code{/} or any subdirectory?
#' @param fallthrough With the default value, \code{FALSE}, if a request is made
#'   for a file that doesn't exist, then httpuv will immediately send a 404
#'   response from the background I/O thread, without needing to call back into
#'   the main R thread. This offers the best performance. If the value is
#'   \code{TRUE}, then instead of sending a 404 response, httpuv will call the
#'   application's \code{call} function, and allow it to handle the request.
#' @export
staticPath <- function(path, indexhtml = TRUE, fallthrough = FALSE) {
  if (!is.character(path) || length(path) != 1 || path == "") {
    stop("`path` must be a non-empty string.")
  }

  path <- normalizePath(path, mustWork = TRUE)  

  structure(
    list(
      path = path,
      indexhtml = indexhtml,
      fallthrough = fallthrough
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
  stop("Cannot convert object of class ", class(path), " to a staticPath object.")
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
    "  Local path:       ", x$path, "\n",
    "  Use index.html:   ", x$indexhtml, "\n",
    "  Fallthrough to R: ", x$fallthrough
  )
}

# This function always returns a named list of staticPath objects. The names
# will all start with "/". The input can be a named character vector or a
# named list containing a mix of strings and staticPath objects.
normalizeStaticPaths <- function(paths) {
  if (is.null(paths) || length(paths) == 0) {
    return(list())
  }

  if (any_unnamed(paths)) {
    stop("paths must be a named character vector, a named list containing strings and staticPath objects, or NULL.")
  }

  if (!is.character(paths) && !is.list(paths)) {
    stop("paths must be a named character vector, a named list containing strings and staticPath objects, or NULL.")
  }

  # Convert to list of staticPath objects. Need this verbose wrapping of
  # as.staticPath because of S3 dispatch for non-registered methods.
  paths <- lapply(paths, function(path) as.staticPath(path))

  # Make sure URL paths have a leading '/' and no trailing '/'.
  names(paths) <- vapply(names(paths), function(path) {
    if (path == "") {
      stop("All paths must be non-empty strings.")
    }
    # Ensure there's a leading / for every path
    if (substr(path, 1, 1) != "/") {
      path <- paste0("/", path)
    }
    # Strip trailing slashes
    path <- sub("/+$", "", path)

    path
  }, "")

  paths
}
