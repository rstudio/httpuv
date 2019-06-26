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

# Given a vector with multiple keys with the same name, drop any duplicated
# names. For example, with an input like list(a=1, a=2), returns list(a=1).
drop_duplicate_names <- function(x) {
  if (any_unnamed(x)) {
    stop("All items must be named.")
  }
  x[unique(names(x))]
}


#' Get and set logging level
#'
#' The logging level for httpuv can be set to report differing levels of
#' information. Possible logging levels (from least to most information
#' reported) are: \code{"OFF"}, \code{"ERROR"}, \code{"WARN"}, \code{"INFO"}, or
#' \code{"DEBUG"}. The default level is \code{ERROR}.
#'
#' @param level The logging level. Must be one of \code{NULL}, \code{"OFF"},
#'   \code{"ERROR"}, \code{"WARN"}, \code{"INFO"}, or \code{"DEBUG"}. If
#'   \code{NULL} (the default), then this function simply returns the current
#'   logging level.
#'
#' @return If \code{level=NULL}, then this returns the current logging level. If
#'   \code{level} is any other value, then this returns the previous logging
#'   level, from before it is set to the new value.
#'
#' @keywords internal
logLevel <- function(level = NULL) {
  if (is.null(level)) {
    level <- ""
    log_level("")
  } else {
    level <- match.arg(level, c("OFF", "ERROR", "WARN", "INFO", "DEBUG"))
    invisible(log_level(level))
  }

}
