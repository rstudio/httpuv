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
