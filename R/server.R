#' @include httpuv.R
NULL

# Note that the methods listed for Server, WebServer, and PipeServer were copied
# and pasted among all three, with a few additional methods added to WebServer
# and PipeServer. When changes are made in the future, make sure that they're
# duplicated among all three.

#' Server class
#'
#' The \code{Server} class is the parent class for \code{\link{WebServer}} and
#' \code{\link{PipeServer}}. This class defines an interface and is not meant to
#' be instantiated.
#' 
#' @section Methods:
#' 
#' \describe{
#'   \item{\code{stop()}}{Stops a running server.}
#'   \item{\code{isRunning()}}{Returns TRUE if the server is currently running.}
#'   \item{\code{getStaticPaths()}}{Returns a list of \code{\link{staticPath}}
#'     objects for the server.
#'   }
#'   \item{\code{setStaticPath(..., .list = NULL)}}{Sets a static path for the
#'     current server. Each static path can be given as a named argument, or as
#'     an named item in \code{.list}. If there already exists a static path with
#'     the same name, it will be replaced.
#'   }
#'   \item{\code{removeStaticPath(path)}}{Removes a static path with the given
#'     name.
#'   }
#'   \item{\code{getStaticPathOptions()}}{Returns a list of default
#'     \code{staticPathOptions} for the current server. Each static path will
#'     use these options by default, but they can be overridden for each static
#'     path.
#'   }
#'   \item{\code{setStaticPathOption(..., .list = NULL)}}{Sets one or more
#'     static path options. Each option can be given as a named argument, or as
#'     a named item in \code{.list}.
#'   }
#' }
#'
#' @keywords internal
#' @importFrom R6 R6Class
Server <- R6Class("Server",
  cloneable = FALSE,
  public = list(
    stop = function() {
      if (!private$running) return(invisible())

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


#' WebServer class
#'
#' This class represents a web server running one application. Multiple servers
#' can be running at the same time.
#' 
#' @section Methods:
#' 
#' \describe{
#'   \item{\code{initialize(host, port, app)}}{
#'     Create a new \code{WebServer} object. \code{app} is an httpuv application
#'     object as described in \code{\link{startServer}}.
#'   }
#'   \item{\code{getHost()}}{Return the value of \code{host} that was passed to
#'     \code{initialize()}.
#'   }
#'   \item{\code{getPort()}}{Return the value of \code{port} that was passed to
#'     \code{initialize()}.
#'   }
#'   \item{\code{stop()}}{Stops a running server.}
#'   \item{\code{isRunning()}}{Returns TRUE if the server is currently running.}
#'   \item{\code{getStaticPaths()}}{Returns a list of \code{\link{staticPath}}
#'     objects for the server.
#'   }
#'   \item{\code{setStaticPath(..., .list = NULL)}}{Sets a static path for the
#'     current server. Each static path can be given as a named argument, or as
#'     an named item in \code{.list}. If there already exists a static path with
#'     the same name, it will be replaced.
#'   }
#'   \item{\code{removeStaticPath(path)}}{Removes a static path with the given
#'     name.
#'   }
#'   \item{\code{getStaticPathOptions()}}{Returns a list of default
#'     \code{staticPathOptions} for the current server. Each static path will
#'     use these options by default, but they can be overridden for each static
#'     path.
#'   }
#'   \item{\code{setStaticPathOption(..., .list = NULL)}}{Sets one or more
#'     static path options. Each option can be given as a named argument, or as
#'     a named item in \code{.list}.
#'   }
#' }
#'
#' @keywords internal
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
    },
    getHost = function() {
      private$host
    },
    getPort = function() {
      private$port
    }
  ),
  private = list(
    host = NULL,
    port = NULL
  )
)


#' PipeServer class
#'
#' This class represents a server running one application that listens on a
#' named pipe.
#' 
#' @section Methods:
#' 
#' \describe{
#'   \item{\code{initialize(name, mask, app)}}{
#'     Create a new \code{PipeServer} object. \code{app} is an httpuv application
#'     object as described in \code{\link{startServer}}.
#'   }
#'   \item{\code{getName()}}{Return the value of \code{name} that was passed to
#'     \code{initialize()}.
#'   }
#'   \item{\code{getMask()}}{Return the value of \code{mask} that was passed to
#'     \code{initialize()}.
#'   }
#'   \item{\code{stop()}}{Stops a running server.}
#'   \item{\code{isRunning()}}{Returns TRUE if the server is currently running.}
#'   \item{\code{getStaticPaths()}}{Returns a list of \code{\link{staticPath}}
#'     objects for the server.
#'   }
#'   \item{\code{setStaticPath(..., .list = NULL)}}{Sets a static path for the
#'     current server. Each static path can be given as a named argument, or as
#'     an named item in \code{.list}. If there already exists a static path with
#'     the same name, it will be replaced.
#'   }
#'   \item{\code{removeStaticPath(path)}}{Removes a static path with the given
#'     name.
#'   }
#'   \item{\code{getStaticPathOptions()}}{Returns a list of default
#'     \code{staticPathOptions} for the current server. Each static path will
#'     use these options by default, but they can be overridden for each static
#'     path.
#'   }
#'   \item{\code{setStaticPathOption(..., .list = NULL)}}{Sets one or more
#'     static path options. Each option can be given as a named argument, or as
#'     a named item in \code{.list}.
#'   }
#' }
#'
#' @keywords internal
PipeServer <- R6Class("PipeServer",
  cloneable = FALSE,
  inherit = Server,
  public = list(
    initialize = function(name, mask, app) {
      if (is.null(mask)) {
        mask <- -1
      }
      private$mask <- mask
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
    },
    getName = function() {
      private$name
    },
    getMask = function() {
      private$mask
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
