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
