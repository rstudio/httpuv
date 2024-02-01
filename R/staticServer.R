#' Serve a directory
#'
#' `runStaticServer()` provides a convenient interface to start a server to host
#' a single static directory, either in the foreground or the background.
#'
#' @examplesIf interactive()
#' website_dir <- system.file("example-static-site", package = "httpuv")
#' runStaticServer(dir = website_dir)
#'
#' @param dir The directory to serve. Defaults to the current working directory.
#' @inheritParams startServer
#' @inheritDotParams staticPath
#' @param background Whether to run the server in the background. By default,
#'   the server runs in the foreground and blocks the R console. You can stop
#'   the server by interrupting it with `Ctrl + C`.
#'
#'   When `background = TRUE`, the server will run in the background and will
#'   process requests when the R console is idle. To stop a background server,
#'   call [stopAllServers()] or call [stopServer()] on the server object
#'   returned (invisibly) by this function.
#' @param browse Whether to automatically open the served directory in a web
#'   browser. Defaults to `TRUE` when running interactively.
#'
#' @returns Starts a server on the specified host and port. By default the
#'   server runs in the foreground and is accessible at `http://127.0.0.1:7446`.
#'   When `background = TRUE`, the `server` object is returned invisibly.
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
  ...,
  background = FALSE,
  browse = interactive()
) {
  root <- staticPath(dir, ...)

  if (is.null(port)) {
    port <- if (is_port_available(7446, host)) 7446 else randomPort(host = host)
  } else {
    stopifnot(
      "`port` must be an integer" = is.numeric(port),
      "`port` must be an integer" = port == as.integer(port),
      "`port` must be a single integer" = length(port) == 1,
      "`port` must be a positive integer" = port > 0,
      "`port` must be less than 65536" = port < 65536
    )

    if (!is_port_available(port, host)) {
      error_unavailable_port(
        paste("Port", port, "is not available on host", host)
      )
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
    tryCatch(
      utils::browseURL(paste0("http://", host, ":", port)),
      error = function(err) {
        warning("Could not open browser due to error in `utils::browseURL()`: ", conditionMessage(err))
      }
    )
  }

  if (isTRUE(background)) {
    return(invisible(server))
  }

  on.exit(stopServer(server))
  message("Press Esc or Ctrl + C to stop the server")
  service(0)
}
