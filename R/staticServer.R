#' Serve a directory
#'
#' Run a server to host a static directory.
#'
#' @examplesIf interactive()
#' website_dir <- system.file("example-static-site", package = "httpuv")
#' runStaticServer(dir = website_dir)
#'
#' @param dir The directory to serve. Defaults to the current working directory.
#' @inheritParams startServer
#' @param background Whether to run the server in the background. By default,
#'   the server runs in the foreground and blocks the R console. You can stop
#'   the server by interrupting it with `Ctrl + C`.
#'
#'   When `background = TRUE`, the server will run in the background and will
#'   process requests when the R console is idle. To stop a background server,
#'   call [stopAllServers()] or call [stopServer()] on the server object
#'   returned (invisibly) by this function.
#'
#' @seealso [runServer()] provides a similar interface for running a dynamic
#'   app server. Both `runStaticServer()` and [runServer()] are built on top of
#'   [startServer()], [service()] and [stopServer()]. Learn more about httpuv
#'   servers in [startServer()].
#'
#' @export
runStaticServer <- function(
  dir = getwd(),
  host = "127.0.0.1",
  port = 7446,
  background = FALSE,
  browse = interactive()
) {
  stopifnot(
    "`port` must be an integer" = is.numeric(port),
    "`port` must be an integer" = port == as.integer(port),
    "`port` must be a single integer" = length(port) == 1
  )

  browse <- isTRUE(browse)
  port <- requested_or_random_port(port, host)
  root <- staticPath(dir)

  server <- startServer(
    app = list(staticPaths = list("/" = root)),
    host = host,
    port = port
  )

  message("Serving: '", dir, "'")
  message("View at: http://", host, ":", port, sep = "")

  if (browse) {
    utils::browseURL(paste0("http://", host, ":", port))
  }

  if (background) return(invisible(server))

  on.exit(stopServer(server))
  message("Press Esc or Ctrl + C to stop the server")
  service(0)
}

requested_or_random_port <- function(port, host = "127.0.0.1") {
  tryCatch(
    # Is the requested port open?
    randomPort(port, port, host = host),
    error = function(err) {
      msg <- conditionMessage(err)
      if (!grepl("Cannot find an available port", msg, fixed = TRUE)) {
        stop(msg)
      }
      warning(
        "Port ", port, " is not available. Using a random port.",
        immediate. = TRUE,
        call. = FALSE
      )
      randomPort(host = host)
    }
  )
}
