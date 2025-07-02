#' @include httpuv.R
NULL

# Note that the methods listed for Server, WebServer, and PipeServer were copied
# and pasted among all three, with a few additional methods added to WebServer
# and PipeServer. When changes are made in the future, make sure that they're
# duplicated among all three.

#' @title Server class
#' @description
#' The `Server` class is the parent class for [WebServer()] and
#' [PipeServer()]. This class defines an interface and is not meant to
#' be instantiated.
#'
#' @seealso [WebServer()] and [PipeServer()].
#' @keywords internal
Server <- R6Class(
  "Server",
  cloneable = FALSE,
  public = list(
    #' @description
    #' Stop a running server
    stop = function() {
      if (!private$running) {
        return(invisible())
      }

      stopServer_(private$handle)
      private$running <- FALSE
      deregisterServer(self)
      invisible()
    },
    #' @description
    #' Check if the server is running
    #' @return TRUE if the server is running, FALSE otherwise.
    isRunning = function() {
      # This doesn't map exactly to whether the app is running, since the
      # server's uv_loop runs on the background thread. This could be changed
      # to something that queries the C++ side about what's running.
      private$running
    },
    #' @description
    #' Get the static paths for the server
    #' @return A list of [staticPath()] objects.
    getStaticPaths = function() {
      if (!private$running) {
        return(NULL)
      }

      getStaticPaths_(private$handle)
    },
    #' @description
    #' Set a static path for the server
    #'
    #' @param ... Named arguments where each name is the name of the static path
    #'   and the value is the path to the directory to serve. If there already
    #'   exists a static path with the same name, it will be replaced.
    #' @param .list A named list where each name is the name of the static path
    #'   and the value is the path to the directory to serve. If there already
    #'   exists a static path with the same name, it will be replaced.
    #' @examples
    #' \dontrun{
    #' # Create a server
    #' server <- WebServer$new("127.0.0.1", 8080, app = my_app)
    #' #' # Set a static path
    #' server$setStaticPath(
    #'   staticPath1 = "path/to/static/files",
    #'   staticPath2 = "another/path/to/static/files"
    #' )
    #' }
    setStaticPath = function(..., .list = NULL) {
      if (!private$running) {
        return(invisible())
      }

      paths <- c(list(...), .list)
      paths <- normalizeStaticPaths(paths)
      invisible(setStaticPaths_(private$handle, paths))
    },
    #' @description
    #' Remove a static path
    #'
    #' @param path The name of the static path to remove.
    #' @return An invisible NULL if the server is running, otherwise it does
    #'   nothing.
    #' @examples
    #' \dontrun{
    #' # Create a server
    #' server <- WebServer$new("127.0.0.1", 8080, app = my_app)
    #' # Set a static path
    #' server$setStaticPath(
    #'   staticPath1 = "path/to/static/files",
    #'   staticPath2 = "another/path/to/static/files"
    #' )
    #' # Remove a static path
    #' server$removeStaticPath("staticPath1")
    #' }
    removeStaticPath = function(path) {
      if (!private$running) {
        return(invisible())
      }

      path <- as.character(path)
      invisible(removeStaticPaths_(private$handle, path))
    },
    #' @description
    #' Get the static path options for the server
    #'
    #' @return A list of default `staticPathOptions` for the current server.
    #'   Each static path will use these options by default, but they can be
    #'   overridden for each static path.
    getStaticPathOptions = function() {
      if (!private$running) {
        return(NULL)
      }

      getStaticPathOptions_(private$handle)
    },
    #' @description
    #' Set one or more static path options
    #'
    #' @param ... Named arguments where each name is the name of the static path
    #'   option and the value is the value to set for that option.
    #' @param .list A named list where each name is the name of the static path
    #'   option and the value is the value to set for that option.
    #' @return An invisible NULL if the server is running, otherwise it does
    #'   nothing.
    setStaticPathOption = function(..., .list = NULL) {
      if (!private$running) {
        return(invisible())
      }

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


#' @title WebServer class
#' @description
#' This class represents a web server running one application. Multiple servers
#' can be running at the same time.
#'
#' @seealso [Server()] and [PipeServer()].
#' @keywords internal
WebServer <- R6Class(
  "WebServer",
  cloneable = FALSE,
  inherit = Server,
  public = list(
    #' @description
    #' Initialize a new WebServer object
    #'
    #' Create a new `WebServer` object. `app` is an httpuv application
    #' object as described in [startServer()].
    #' @param host The host name or IP address to bind the server to.
    #' @param port The port number to bind the server to.
    #' @param app An httpuv application object as described in [startServer()].
    #' @param quiet If TRUE, suppresses output from the server.
    #' @return A new `WebServer` object.
    #' @examples
    #' \dontrun{
    #' # Create a simple app
    #' app <- function(req) {
    #'   list(
    #'     status = 200L,
    #'     headers = list('Content-Type' = 'text/plain'),
    #'     body = "Hello, world!"
    #'   )
    #' }
    #' # Create a server
    #' server <- WebServer$new("127.0.0.1", 8080, app)
    #' }
    initialize = function(host, port, app, quiet = FALSE) {
      private$host <- host
      private$port <- port
      private$appWrapper <- AppWrapper$new(app)

      private$handle <- makeTcpServer(
        host,
        port,
        private$appWrapper$onHeaders,
        private$appWrapper$onBodyData,
        private$appWrapper$call,
        private$appWrapper$onWSOpen,
        private$appWrapper$onWSMessage,
        private$appWrapper$onWSClose,
        private$appWrapper$staticPaths,
        private$appWrapper$staticPathOptions,
        quiet
      )

      if (is.null(private$handle)) {
        stop("Failed to create server")
      }

      private$running <- TRUE
      registerServer(self)
    },
    #' @description
    #' Get the host name or IP address of the server
    #'
    #' @return The host name or IP address that the server is bound to.
    getHost = function() {
      private$host
    },
    #' @description
    #' Get the port number of the server
    #'
    #' @return The port number that the server is bound to.
    getPort = function() {
      private$port
    }
  ),
  private = list(
    host = NULL,
    port = NULL
  )
)


#' @title PipeServer class
#' @description
#' This class represents a server running one application that listens on a
#' named pipe.
#'
#' @seealso [Server()] and [WebServer()].
#' @keywords internal
PipeServer <- R6Class(
  "PipeServer",
  cloneable = FALSE,
  inherit = Server,
  public = list(
    #' @description
    #' Initialize a new PipeServer object
    #'
    #' Create a new `PipeServer` object. `app` is an httpuv application
    #' object as described in [startServer()].
    #' @param name The name of the named pipe to bind the server to.
    #' @param mask The mask for the named pipe. If NULL, it defaults to -1.
    #' @param app An httpuv application object as described in
    #'   [startServer()].
    #' @param quiet If TRUE, suppresses output from the server.
    #' @return A new `PipeServer` object.
    #' @examples
    #' \dontrun{
    #' # Create a simple app
    #' app <- function(req) {
    #'   list(
    #'     status = 200L,
    #'     headers = list('Content-Type' = 'text/plain'),
    #'     body = "Hello, world!"
    #'   )
    #' }
    #' # Create a server
    #' server <- PipeServer$new("my_pipe", -1, app)
    #' }
    initialize = function(name, mask, app, quiet = FALSE) {
      if (is.null(mask)) {
        mask <- -1
      }
      private$mask <- mask
      private$appWrapper <- AppWrapper$new(app)

      private$handle <- makePipeServer(
        name,
        mask,
        private$appWrapper$onHeaders,
        private$appWrapper$onBodyData,
        private$appWrapper$call,
        private$appWrapper$onWSOpen,
        private$appWrapper$onWSMessage,
        private$appWrapper$onWSClose,
        private$appWrapper$staticPaths,
        private$appWrapper$staticPathOptions,
        quiet
      )

      # Save the full path. normalizePath must be called after makePipeServer
      private$name <- normalizePath(name)

      if (is.null(private$handle)) {
        stop("Failed to create server")
      }
    },
    #' @description
    #' Get the name of the named pipe
    #'
    #' @return The name of the named pipe that the server is bound to.
    getName = function() {
      private$name
    },
    #' @description
    #' Get the mask for the named pipe
    #'
    #' @return The mask for the named pipe that the server is bound to.
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
#' [startServer()] or [startPipeServer()], this closes all
#' open connections for that server and unbinds the port.
#'
#' @param server A server object that was previously returned from
#'   [startServer()] or [startPipeServer()].
#'
#' @seealso [stopAllServers()] to stop all servers.
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
#' [startServer()] or [startPipeServer()].
#'
#' @seealso [stopServer()] to stop a specific server.
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

  warning(
    "Unable to deregister server: server not found in list of running servers."
  )
}


#' Stop a running daemonized server in Unix environments (deprecated)
#'
#' This function will be removed in a future release of httpuv. Instead, use
#' [stopServer()].
#'
#' @inheritParams stopServer
#'
#' @export
stopDaemonizedServer <- stopServer
