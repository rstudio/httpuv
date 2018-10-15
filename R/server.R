#' @include httpuv.R

#' @importFrom R6 R6Class
Server <- R6Class("Server",
  cloneable = FALSE,
  public = list(
    stop = function() {
      if (!private$running) {
        stop("Server is already stopped.")
      }
      stopServer_(private$handle)
      private$running <- FALSE
      deregisterServer(self)
    },
    isRunning = function() {
      # This doesn't map exactly to whether the app is running, since the
      # server's uv_loop runs on the background thread. This could be changed
      # to something that queries the C++ side about what's running.
      private$running
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
    handle = NULL,
    running = FALSE
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

      private$running <- TRUE
      registerServer(self)
    },
    getID = function() {
      paste0(private$host, ":", private$port)
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

      # Save the full path. normalizePath must be called after makePipeServer 
      private$name <- normalizePath(name)

      if (is.null(private$handle)) {
        stop("Failed to create server")
      }
    },
    getID = function() {
      private$name
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


#' Stop all servers
#'
#' This will stop all applications which were created by
#' \code{\link{startServer}} or \code{\link{startPipeServer}}.
#'
#' @seealso \code{\link{stopServer}} to stop a specific server.
#'
#' @export
stopAllServers <- function() {
  lapply(.globals$servers, function(server) {
    server$stop()
  })
  invisible()
}


.globals$servers <- list()

#' @export
listServers <- function() {
  .globals$servers
}

registerServer <- function(server) {
  id <- server$getID()
  .globals$servers[[id]] <- server
}

deregisterServer <- function(server) {
  if (is.character(server)) {
    id <- server
  } else {
    # Assume it's a Server object
    id <- server$getID()
  }
  .globals$servers[[id]] <- NULL
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
