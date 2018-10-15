# A memoized wrapper for packageVersion(), because it is a fairly slow function
# which is called often. We can't get the version at build time because the
# package won't have been installed yet. Instead, we'll get it at run time and
# cache it.
httpuv_version <- local({
  version <- NULL

  function() {
    if (is.null(version)) {
      version <<- utils::packageVersion("httpuv")
    }
    version
  }
})


# Return a zero-element named character vector
empty_named_vec <- function() {
  c(a = "")[0]
}

# Given a vector/list, return TRUE if any elements are unnamed, FALSE otherwise.
any_unnamed <- function(x) {
  # Zero-length vector
  if (length(x) == 0) return(FALSE)

  nms <- names(x)

  # List with no name attribute
  if (is.null(nms)) return(TRUE)

  # List with name attribute; check for any ""
  any(!nzchar(nms))
}


# This function always returns a named character vector of path mappings. The
# names will all start with "/".
normalizeStaticPaths <- function(paths) {
  if (is.null(paths) || length(paths) == 0) {
    return(empty_named_vec())
  }

  if (any_unnamed(paths)) {
    stop("paths must be a named character vector, a named list of strings, or NULL.")
  }

  # If list, convert to vector.
  if (is.list(paths)) {
    paths <- unlist(paths, recursive = FALSE)
  }

  if (!is.character(paths)) {
    stop("paths must be a named character vector, a named list of strings, or NULL.")
  }

  # If we got here, it is a named character vector.

  # Make sure paths have a leading '/'. Save in a separate var because
  # we later call normalizePath(), which drops names.
  path_names <- vapply(names(paths), function(path) {
    if (path == "") {
      stop("All paths must be non-empty strings.")
    }
    # Ensure there's a leading / for every path
    if (substr(path, 1, 1) != "/") {
      path <- paste0("/", path)
    }
    path
  }, "")

  paths <- normalizePath(paths, mustWork = TRUE)
  names(paths) <- path_names

  paths
}