#' @importFrom R6 R6Class
Server <- R6Class("Server",
  cloneable = FALSE,
  public = list(
    stop = function() {
      # TODO: what if is already running?
      stopServer_(private$handle)
    },
    getStaticPaths = function() {
      getStaticPaths_(private$handle)
    },
    addStaticPaths = function(paths) {
      paths <- normalizeStaticPaths(paths)
      invisible(addStaticPaths_(private$handle, paths))
    },
    removeStaticPaths = function(paths) {
      paths <- as.character(paths)
      invisible(removeStaticPaths_(private$handle, paths))
    }
  ),
  private = list(
    appWrapper = NULL,
    handle = NULL
  )
)


#' @export
WebServer <- R6Class("WebServer",
  cloneable = FALSE,
  inherit = Server,
  public = list(
    initialize = function(host, port, app) {
      private$host <- host
      private$port <- port
      private$appWrapper <- AppWrapper$new(app)

      private$handle <- makeTcpServer(
        host, port,
        private$appWrapper$onHeaders,
        private$appWrapper$onBodyData,
        private$appWrapper$call,
        private$appWrapper$onWSOpen,
        private$appWrapper$onWSMessage,
        private$appWrapper$onWSClose,
        private$appWrapper$getStaticPaths
      )

      if (is.null(private$handle)) {
        stop("Failed to create server")
      }
    }
  ),
  private = list(
    host = NULL,
    port = NULL
  )
)


#' @export
PipeServer <- R6Class("PipeServer",
  cloneable = FALSE,
  inherit = Server,
  public = list(
    initialize = function(name, mask, app) {
      private$name <- name
      if (is.null(private$mask)) {
        private$mask <- -1
      }
      private$appWrapper <- AppWrapper$new(app)

      private$handle <- makePipeServer(
        name, mask,
        private$appWrapper$onHeaders,
        private$appWrapper$onBodyData,
        private$appWrapper$call,
        private$appWrapper$onWSOpen,
        private$appWrapper$onWSMessage,
        private$appWrapper$onWSClose
      )
      if (is.null(private$handle)) {
        stop("Failed to create server")
      }
    }
  ),
  private = list(
    name = NULL,
    mask = NULL
  )
)


#' Stop a server
#' 
#' Given a handle that was returned from a previous invocation of 
#' \code{\link{startServer}} or \code{\link{startPipeServer}}, this closes all
#' open connections for that server and  unbinds the port.
#' 
#' @param handle A handle that was previously returned from
#'   \code{\link{startServer}} or \code{\link{startPipeServer}}.
#'
#' @seealso \code{\link{stopAllServers}} to stop all servers.
#'
#' @export
stopServer <- function(server) {
  if (!inherits(server, "Server")) {
    stop("Object must be an object of class Server.")
  }
  server$stop()
}


#' Stop a running daemonized server in Unix environments (deprecated)
#'
#' This function will be removed in a future release of httpuv. Instead, use
#' \code{\link{stopServer}}.
#'
#' @inheritParams stopServer
#'
#' @export
stopDaemonizedServer <- stopServer
