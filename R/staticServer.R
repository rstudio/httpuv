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
  port = NULL,
  background = FALSE,
  browse = interactive()
) {
  root <- staticPath(dir)

  if (is.null(port)) {
    port <- if (is_port_available(7446, host)) 7446 else randomPort(host = host)
  } else {
    stopifnot(
      "`port` must be an integer" = is.numeric(port),
      "`port` must be an integer" = port == as.integer(port),
      "`port` must be a single integer" = length(port) == 1
    )

    if (!is_port_available(port, host)) {
      error_unavailable_port(paste0("Port ", port, " is not available"))
    }
  }

  server <- startServer(
    app = list(staticPaths = list("/" = root)),
    host = host,
    port = port
  )

  message("Serving: '", dir, "'")
  message("View at: http://", host, ":", port, sep = "")

  if (isTRUE(browse)) {
    utils::browseURL(paste0("http://", host, ":", port))
  }

  if (isTRUE(background)) {
    return(invisible(server))
  }

  on.exit(stopServer(server))
  message("Press Esc or Ctrl + C to stop the server")
  service(0)
}
