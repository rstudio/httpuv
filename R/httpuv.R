#' HTTP and WebSocket server
#' 
#' Allows R code to listen for and interact with HTTP and WebSocket clients, so 
#' you can serve web traffic directly out of your R process. Implementation is
#' based on \href{https://github.com/joyent/libuv}{libuv} and
#' \href{https://github.com/joyent/http-parser}{http-parser}.
#' 
#' This is a low-level library that provides little more than network I/O and 
#' implementations of the HTTP and WebSocket protocols. For an easy way to 
#' create web applications, try \href{http://rstudio.com/shiny/}{Shiny} instead.
#' 
#' @examples
#' \dontrun{
#' demo("echo", package="httpuv")
#' }
#' 
#' @seealso startServer
#'   
#' @name httpuv-package
#' @aliases httpuv
#' @docType package
#' @title HTTP and WebSocket server
#' @author Joe Cheng \email{joe@@rstudio.com}
#' @keywords package
#' @useDynLib httpuv
#' @import methods
#' @importFrom Rcpp evalCpp
#' @importFrom utils packageVersion
NULL

# Implementation of Rook input stream
InputStream <- setRefClass(
  'InputStream',
  fields = list(
    .conn = 'ANY',
    .length = 'numeric'
  ),
  methods = list(
    initialize = function(conn, length) {
      .conn <<- conn
      .length <<- length
      seek(.conn, 0)
    },
    read_lines = function(n = -1L) {
      readLines(.conn, n, warn=FALSE)
    },
    read = function(l = -1L) {
      # l < 0 means read all remaining bytes
      if (l < 0)
        l <- .length - seek(.conn)
      
      if (l == 0)
        return(raw())
      else
        return(readBin(.conn, raw(), l))
    },
    rewind = function() {
      seek(.conn, 0)
    }
  )
)

NullInputStream <- setRefClass(
  'NullInputStream',
  methods = list(
    read_lines = function(n = -1L) {
      character()
    },
    read = function(l = -1L) {
      raw()
    },
    rewind = function() invisible(),
    close = function() invisible()
  )
)
nullInputStream <- NullInputStream$new()

#Implementation of Rook error stream
ErrorStream <- setRefClass(
  'ErrorStream',
  methods = list(
    cat = function(... , sep = " ", fill = FALSE, labels = NULL) {
      base::cat(..., sep=sep, fill=fill, labels=labels, file=stderr())
    },
    flush = function() {
      base::flush(stderr())
    }
  )
)

rookCall <- function(func, req, data = NULL, dataLength = -1) {
  result <- try({
    inputStream <- if(is.null(data))
      nullInputStream
    else
      InputStream$new(data, dataLength)

    req$rook.input <- inputStream
    
    req$rook.errors <- ErrorStream$new()
    
    req$httpuv.version <- packageVersion('httpuv')
    
    # These appear to be required for Rook multipart parsing to work
    if (!is.null(req$HTTP_CONTENT_TYPE))
      req$CONTENT_TYPE <- req$HTTP_CONTENT_TYPE
    if (!is.null(req$HTTP_CONTENT_LENGTH))
      req$CONTENT_LENGTH <- req$HTTP_CONTENT_LENGTH
    
    resp <- func(req)

    if (is.null(resp) || length(resp) == 0)
      return(NULL)
    
    # Coerce all headers to character
    resp$headers <- lapply(resp$headers, paste)
    
    if ('file' %in% names(resp$body)) {
      filename <- resp$body[['file']]
      owned <- FALSE
      if ('owned' %in% names(resp$body))
        owned <- as.logical(resp$body$owned)

      resp$body <- NULL
      resp$bodyFile <- filename
      resp$bodyFileOwned <- owned
    }
    resp
  })
  if (inherits(result, 'try-error')) {
    return(list(
      status=500L,
      headers=list(
        'Content-Type'='text/plain; charset=UTF-8'
      ),
      body=charToRaw(enc2utf8(
        paste("ERROR:", attr(result, "condition")$message, collapse="\n")
      ))
    ))
  } else {
    return(result)
  }
}

AppWrapper <- setRefClass(
  'AppWrapper',
  fields = list(
    .app = 'ANY',
    .wsconns = 'environment',
    .supportsOnHeaders = 'logical'
  ),
  methods = list(
    initialize = function(app) {
      if (is.function(app))
        .app <<- list(call=app)
      else
        .app <<- app
      
      # .app$onHeaders can error (e.g. if .app is a reference class)
      .supportsOnHeaders <<- isTRUE(try(!is.null(.app$onHeaders), silent=TRUE))
    },
    onHeaders = function(req) {
      if (!.supportsOnHeaders)
        return(NULL)

      rookCall(.app$onHeaders, req)
    },
    onBodyData = function(req, bytes) {
      if (is.null(req$.bodyData))
        req$.bodyData <- file(open='w+b', encoding='UTF-8')
      writeBin(bytes, req$.bodyData)
    },
    call = function(req) {
      on.exit({
        if (!is.null(req$.bodyData)) {
          close(req$.bodyData)
        }
        req$.bodyData <- NULL
      })
      rookCall(.app$call, req, req$.bodyData, seek(req$.bodyData))
    },
    onWSOpen = function(handle, req) {
      ws <- WebSocket$new(handle, req)
      .wsconns[[as.character(handle)]] <<- ws
      result <- try(.app$onWSOpen(ws))
      
      # If an unexpected error happened, just close up
      if (inherits(result, 'try-error')) {
        # TODO: Close code indicating error?
        ws$close()
      }
    },
    onWSMessage = function(handle, binary, message) {
      for (handler in .wsconns[[as.character(handle)]]$.messageCallbacks) {
        result <- try(handler(binary, message))
        if (inherits(result, 'try-error')) {
          # TODO: Close code indicating error?
          .wsconns[[as.character(handle)]]$close()
          return()
        }
      }
    },
    onWSClose = function(handle) {
      ws <- .wsconns[[as.character(handle)]]
      ws$.handle <- NULL
      rm(list=as.character(handle), pos=.wsconns)
      for (handler in ws$.closeCallbacks) {
        handler()
      }
    }
  )
)

#' WebSocket object
#' 
#' An object that represents a single WebSocket connection. The object can be
#' used to send messages and close the connection, and to receive notifications
#' when messages are received or the connection is closed.
#' 
#' WebSocket objects should never be created directly. They are obtained by
#' passing an \code{onWSOpen} function to \code{\link{startServer}}.
#' 
#' @section Fields:
#'
#'   \describe{
#'     \item{\code{request}}{
#'       The Rook request environment that opened the connection. This can be
#'       used to inspect HTTP headers, for example.
#'     }
#'   }
#'
#' 
#' @section Methods:
#' 
#'   \describe{
#'     \item{\code{onMessage(func)}}{
#'       Registers a callback function that will be invoked whenever a message
#'       is received on this connection. The callback function will be invoked
#'       with two arguments. The first argument is \code{TRUE} if the message
#'       is binary and \code{FALSE} if it is text. The second argument is either
#'       a raw vector (if the message is binary) or a character vector.
#'     }
#'     \item{\code{onClose(func)}}{
#'       Registers a callback function that will be invoked when the connection
#'       is closed.
#'     }
#'     \item{\code{send(message)}}{
#'       Begins sending the given message over the websocket. The message must
#'       be either a raw vector, or a single-element character vector that is
#'       encoded in UTF-8.
#'     }
#'     \item{\code{close()}}{
#'       Closes the websocket connection.
#'     }
#'   }
#'
#' @param ... For internal use only.
#' 
#' @export
WebSocket <- setRefClass(
  'WebSocket',
  fields = list(
    '.handle' = 'ANY',
    '.messageCallbacks' = 'list',
    '.closeCallbacks' = 'list',
    'request' = 'environment'
  ),
  methods = list(
    initialize = function(handle, req) {
      .handle <<- handle
      request <<- req
    },
    onMessage = function(func) {
      .messageCallbacks <<- c(.messageCallbacks, func)
    },
    onClose = function(func) {
      .closeCallbacks <<- c(.closeCallbacks, func)
    },
    send = function(message) {
      if (is.null(.handle))
        stop("Can't send message on a closed WebSocket")
      
      if (is.raw(message))
        sendWSMessage(.handle, TRUE, message)
      else {
        # TODO: Ensure that message is UTF-8 encoded
        sendWSMessage(.handle, FALSE, as.character(message))
      }
    },
    close = function() {
      if (is.null(.handle))
        return()
      
      closeWS(.handle)
    }
  )
)

#' Create an HTTP/WebSocket server
#' 
#' Creates an HTTP/WebSocket server on the specified host and port.
#' 
#' @param host A string that is a valid IPv4 address that is owned by this 
#'   server, or \code{"0.0.0.0"} to listen on all IP addresses.
#' @param port A number or integer that indicates the server port that should be
#'   listened on. Note that on most Unix-like systems including Linux and Mac OS
#'   X, port numbers smaller than 1025 require root privileges.
#' @param app A collection of functions that define your application. See 
#'   Details.
#' @return A handle for this server that can be passed to
#'   \code{\link{stopServer}} to shut the server down.
#'   
#' @details \code{startServer} binds the specified port, but no connections are 
#'   actually accepted. See \code{\link{service}}, which should be called 
#'   repeatedly in order to actually accept and handle connections. If the port
#'   cannot be bound (most likely due to permissions or because it is already
#'   bound), an error is raised.
#'   
#'   The \code{app} parameter is where your application logic will be provided 
#'   to the server. This can be a list, environment, or reference class that 
#'   contains the following named functions/methods:
#'   
#'   \describe{
#'     \item{\code{call(req)}}{Process the given HTTP request, and return an 
#'     HTTP response. This method should be implemented in accordance with the
#'     \href{https://github.com/jeffreyhorner/Rook/blob/a5e45f751/README.md}{Rook}
#'     specification.}
#'     \item{\code{onHeaders(req)}}{Optional. Similar to \code{call}, but occurs
#'     when headers are received. Return \code{NULL} to continue normal
#'     processing of the request, or a Rook response to send that response,
#'     stop processing the request, and ask the client to close the connection.
#'     (This can be used to implement upload size limits, for example.)}
#'     \item{\code{onWSOpen(ws)}}{Called back when a WebSocket connection is established.
#'     The given object can be used to be notified when a message is received from
#'     the client, to send messages to the client, etc. See \code{\link{WebSocket}}.}
#'   }
#'   
#'   The \code{startPipeServer} variant can be used instead of 
#'   \code{startServer} to listen on a Unix domain socket or named pipe rather
#'   than a TCP socket (this is not common).
#' @seealso \code{\link{runServer}}
#' @aliases startPipeServer
#' @export
startServer <- function(host, port, app) {
  
  appWrapper <- AppWrapper$new(app)
  server <- makeTcpServer(host, port,
                          appWrapper$onHeaders,
                          appWrapper$onBodyData,
                          appWrapper$call,
                          appWrapper$onWSOpen,
                          appWrapper$onWSMessage,
                          appWrapper$onWSClose)
  if (is.null(server)) {
    stop("Failed to create server")
  }
  return(server)
}

#' @param name A string that indicates the path for the domain socket (on 
#'   Unix-like systems) or the name of the named pipe (on Windows).
#' @param mask If non-\code{NULL} and non-negative, this numeric value is used 
#'   to temporarily modify the process's umask while the domain socket is being 
#'   created. To ensure that only root can access the domain socket, use 
#'   \code{strtoi("777", 8)}; or to allow owner and group read/write access, use
#'   \code{strtoi("117", 8)}. If the value is \code{NULL} then the process's
#'   umask is left unchanged. (This parameter has no effect on Windows.)
#' @rdname startServer
#' @export
startPipeServer <- function(name, mask, app) {
  
  appWrapper <- AppWrapper$new(app)
  if (is.null(mask))
    mask <- -1
  server <- makePipeServer(name, mask,
                           appWrapper$onHeaders,
                           appWrapper$onBodyData,
                           appWrapper$call,
                           appWrapper$onWSOpen,
                           appWrapper$onWSMessage,
                           appWrapper$onWSClose)
  if (is.null(server)) {
    stop("Failed to create server")
  }
  return(server)
}

#' Process requests
#' 
#' Process HTTP requests and WebSocket messages. Even if a server exists, no
#' requests are serviced unless and until \code{service} is called.
#' 
#' Note that while \code{service} is waiting for a new request, the process is
#' not interruptible using normal R means (Esc, Ctrl+C, etc.). If being
#' interruptible is a requirement, then call \code{service} in a while loop
#' with a very short but non-zero \code{\link{Sys.sleep}} during each iteration.
#' 
#' @param timeoutMs Approximate number of milliseconds to run before returning. 
#'   If 0, then the function will continually process requests without returning
#'   unless an error occurs.
#'
#' @examples
#' \dontrun{
#' while (TRUE) {
#'   service()
#'   Sys.sleep(0.001)
#' }
#' }
#' 
#' @export
service <- function(timeoutMs = ifelse(interactive(), 100, 1000)) {
  run(timeoutMs)
}

#' Stop a running server
#' 
#' Given a handle that was returned from a previous invocation of 
#' \code{\link{startServer}}, closes all open connections for that server and 
#' unbinds the port. \strong{Be careful not to call \code{stopServer} more than 
#' once on a handle, as this will cause the R process to crash!}
#' 
#' @param handle A handle that was previously returned from
#'   \code{\link{startServer}}.
#'   
#' @export
stopServer <- function(handle) {
  destroyServer(handle)
}

#' Run a server
#' 
#' This is a convenience function that provides a simple way to call 
#' \code{\link{startServer}}, \code{\link{service}}, and 
#' \code{\link{stopServer}} in the correct sequence. It does not return unless 
#' interrupted or an error occurs.
#' 
#' If you have multiple hosts and/or ports to listen on, call the individual 
#' functions instead of \code{runServer}.
#' 
#' @param host A string that is a valid IPv4 address that is owned by this 
#'   server, or \code{"0.0.0.0"} to listen on all IP addresses.
#' @param port A number or integer that indicates the server port that should be
#'   listened on. Note that on most Unix-like systems including Linux and Mac OS
#'   X, port numbers smaller than 1025 require root privileges.
#' @param app A collection of functions that define your application. See 
#'   \code{\link{startServer}}.
#' @param interruptIntervalMs How often to check for interrupt. The default 
#'   should be appropriate for most situations.
#'   
#' @seealso \code{\link{startServer}}, \code{\link{service}},
#'   \code{\link{stopServer}}
#' @export
runServer <- function(host, port, app,
                      interruptIntervalMs = ifelse(interactive(), 100, 1000)) {
  server <- startServer(host, port, app)
  on.exit(stopServer(server))
  
  .globals$stopped <- FALSE
  while (!.globals$stopped) {
    service(interruptIntervalMs)
    Sys.sleep(0.001)
  }
}

#' Interrupt httpuv runloop
#' 
#' Interrupts the currently running httpuv runloop, meaning
#' \code{\link{runServer}} or \code{\link{service}} will return control back to
#' the caller and no further tasks will be processed until those methods are
#' called again. Note that this may cause in-process uploads or downloads to be
#' interrupted in mid-request.
#' 
#' @export
interrupt <- function() {
  stopLoop()
  .globals$stopped <- TRUE
}

#' Convert raw vector to Base64-encoded string
#' 
#' Converts a raw vector to its Base64 encoding as a single-element character
#' vector.
#' 
#' @param x A raw vector.
#'   
#' @examples
#' set.seed(100)
#' result <- rawToBase64(as.raw(runif(19, min=0, max=256)))
#' stopifnot(identical(result, "TkGNDnd7z16LK5/hR2bDqzRbXA=="))
#' 
#' @export
rawToBase64 <- function(x) {
  base64encode(x)
}

.globals <- new.env()


#' Create an HTTP/WebSocket daemonized server (experimental)
#' 
#' Creates an HTTP/WebSocket server on the specified host and port. The server is daemonized
#' so R interactive sessions are not blocked to handle requests.
#' 
#' @param host A string that is a valid IPv4 address that is owned by this 
#'   server, or \code{"0.0.0.0"} to listen on all IP addresses.
#' @param port A number or integer that indicates the server port that should be
#'   listened on. Note that on most Unix-like systems including Linux and Mac OS
#'   X, port numbers smaller than 1025 require root privileges.
#' @param app A collection of functions that define your application. See 
#'   Details.
#' @return A handle for this server that can be passed to
#'   \code{\link{stopDaemonizedServer}} to shut the server down.
#'   
#' @details In contrast to servers created by \code{\link{startServer}}, calls to \code{\link{service}} 
#'   are not needed to accept and handle connections. If the port
#'   cannot be bound (most likely due to permissions or because it is already
#'   bound), an error is raised.
#'   
#'   The \code{app} parameter is where your application logic will be provided 
#'   to the server. This can be a list, environment, or reference class that 
#'   contains the following named functions/methods:
#'   
#'   \describe{
#'     \item{\code{call(req)}}{Process the given HTTP request, and return an 
#'     HTTP response. This method should be implemented in accordance with the
#'     \href{https://github.com/jeffreyhorner/Rook/blob/a5e45f751/README.md}{Rook}
#'     specification.}
#'     \item{\code{onHeaders(req)}}{Optional. Similar to \code{call}, but occurs
#'     when headers are received. Return \code{NULL} to continue normal
#'     processing of the request, or a Rook response to send that response,
#'     stop processing the request, and ask the client to close the connection.
#'     (This can be used to implement upload size limits, for example.)}
#'     \item{\code{onWSOpen(ws)}}{Called back when a WebSocket connection is established.
#'     The given object can be used to be notified when a message is received from
#'     the client, to send messages to the client, etc. See \code{\link{WebSocket}}.}
#'   }
#'   
#'   The \code{startPipeServer} variant is not supported yet.
#' @seealso \code{\link{startServer}}
#' @export
startDaemonizedServer <- function(host, port, app) {
  server <- startServer(host, port, app)
  tryCatch({
    server <- daemonize(server)
  }, error=function(e) {
    stopServer(server)
    stop(e)
  })
  return(server)
}

#' Stop a running daemonized server in Unix environments
#' 
#' Given a handle that was returned from a previous invocation of 
#' \code{\link{startDaemonizedServer}}, closes all open connections for that server,
#' removes listeners in the R event loop and 
#' unbinds the port. \strong{Be careful not to call \code{stopDaemonizedServer} more than 
#' once on a handle, as this will cause the R process to crash!}
#' 
#' @param server A handle that was previously returned from
#'   \code{\link{startDaemonizedServer}}.
#'   
#' @export
stopDaemonizedServer <- function(server) {
  destroyDaemonizedServer(server)
}
