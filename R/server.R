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
      invisible()
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
    setStaticPath = function(..., .list = NULL) {
      paths <- c(list(...), .list)
      paths <- drop_duplicate_names(paths)
      paths <- normalizeStaticPaths(paths)
      invisible(setStaticPaths_(private$handle, paths))
    },
    removeStaticPath = function(path) {
      path <- as.character(path)
      invisible(removeStaticPaths_(private$handle, path))
    },
    getStaticPathOptions = function() {
      getStaticPathOptions_(private$handle)
    },
    setStaticPathOption = function(..., .list = NULL) {
      opts <- c(list(...), .list)
      opts <- drop_duplicate_names(opts)
      opts <- normalizeStaticPathOptions(opts)

      unknown_opt_idx <- !(names(opts) %in% names(formals(staticPathOptions)))
      if (any(unknown_opt_idx)) {
        stop("Unknown options: ", paste(names(opts)[unknown_opt_idx], ", "))
      }

      invisible(setStaticPathOptions_(private$handle, opts))
    }
  ),
  private = list(
    appWrapper = NULL,
    handle = NULL,
    running = FALSE
  )
)


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
        private$appWrapper$.staticPaths,
        private$appWrapper$.staticPathOptions
      )

      if (is.null(private$handle)) {
        stop("Failed to create server")
      }

      private$running <- TRUE
      registerServer(self)
    }
  ),
  private = list(
    host = NULL,
    port = NULL
  )
)


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
        private$appWrapper$onWSClose,
        private$appWrapper$.staticPaths,
        private$appWrapper$.staticPathOptions
      )

      # Save the full path. normalizePath must be called after makePipeServer 
      private$name <- normalizePath(name)

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
#' Given a server object that was returned from a previous invocation of
#' \code{\link{startServer}} or \code{\link{startPipeServer}}, this closes all
#' open connections for that server and unbinds the port.
#'
#' @param server A server object that was previously returned from
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

#' List all running httpuv servers
#'
#' This returns a list of all running httpuv server applications.
#'
#' @export
listServers <- function() {
  .globals$servers
}

registerServer <- function(server) {
  .globals$servers[[length(.globals$servers) + 1]] <- server
}

deregisterServer <- function(server) {
  for (i in seq_along(.globals$servers)) {
    if (identical(server, .globals$servers[[i]])) {
      .globals$servers[[i]] <- NULL
      return()
    }
  }

  warning("Unable to deregister server: server not found in list of running servers.")
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
