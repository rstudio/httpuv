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
#' @seealso \link{startServer}
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

#' @importFrom promises promise then finally is.promise %...>% %...!%
rookCall <- function(func, req, data = NULL, dataLength = -1) {

  # Break the processing into two parts: first, the computation with func();
  # second, the preparation of the response object.
  compute <- function() {
    inputStream <- if(is.null(data))
      nullInputStream
    else
      InputStream$new(data, dataLength)

    req$rook.input <- inputStream

    req$rook.errors <- ErrorStream$new()

    req$httpuv.version <- httpuv_version()

    # These appear to be required for Rook multipart parsing to work
    if (!is.null(req$HTTP_CONTENT_TYPE))
      req$CONTENT_TYPE <- req$HTTP_CONTENT_TYPE
    if (!is.null(req$HTTP_CONTENT_LENGTH))
      req$CONTENT_LENGTH <- req$HTTP_CONTENT_LENGTH

    # func() may return a regular value or a promise.
    func(req)
  }

  prepare_response <- function(resp) {
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
  }

  on_error <- function(e) {
    list(
      status=500L,
      headers=list(
        'Content-Type'='text/plain; charset=UTF-8'
      ),
      body=charToRaw(enc2utf8(
        paste("ERROR:", conditionMessage(e), collapse="\n")
      ))
    )
  }

  # First, run the compute function. If it errored, return error response.
  # Then check if it returned a promise. If so, promisify the next step.
  # If not, run the next step immediately.
  compute_error <- NULL
  response <- tryCatch(
    compute(),
    error = function(e) compute_error <<- e
  )
  if (!is.null(compute_error)) {
    return(on_error(compute_error))
  }

  if (is.promise(response)) {
    response %...>% prepare_response %...!% on_error
  } else {
    tryCatch(prepare_response(response), error = on_error)
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
    call = function(req, cpp_callback) {
      # The cpp_callback is an external pointer to a C++ function that writes
      # the response.

      resp <- rookCall(.app$call, req, req$.bodyData, seek(req$.bodyData))
      # Note: rookCall() should never throw error because all the work is
      # wrapped in tryCatch().

      clean_up <- function() {
        if (!is.null(req$.bodyData)) {
          close(req$.bodyData)
        }
        req$.bodyData <- NULL
      }

      if (is.promise(resp)) {
        # Slower path if resp is a promise
        resp <- resp %...>% invokeCppCallback(., cpp_callback)
        finally(resp, clean_up)

      } else {
        # Fast path if resp is a regular value
        on.exit(clean_up())
        invokeCppCallback(resp, cpp_callback)
      }

      invisible()
    },
    onWSOpen = function(handle, req) {
      ws <- WebSocket$new(handle, req)
      .wsconns[[wsconn_address(handle)]] <<- ws
      result <- try(.app$onWSOpen(ws))

      # If an unexpected error happened, just close up
      if (inherits(result, 'try-error')) {
        ws$close(1011, "Error in onWSOpen")
      }
    },
    onWSMessage = function(handle, binary, message) {
      for (handler in .wsconns[[wsconn_address(handle)]]$.messageCallbacks) {
        result <- try(handler(binary, message))
        if (inherits(result, 'try-error')) {
          .wsconns[[wsconn_address(handle)]]$close(1011, "Error executing onWSMessage")
          return()
        }
      }
    },
    onWSClose = function(handle) {
      ws <- .wsconns[[wsconn_address(handle)]]
      ws$.handle <- NULL
      .wsconns[[wsconn_address(handle)]] <<- NULL

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
        return()
      
      if (is.raw(message))
        sendWSMessage(.handle, TRUE, message)
      else {
        # TODO: Ensure that message is UTF-8 encoded
        sendWSMessage(.handle, FALSE, as.character(message))
      }
    },
    close = function(code = 1000L, reason = "") {
      if (is.null(.handle))
        return()

      # Make sure the code will fit in a short int (2 bytes); if not just use
      # "Going Away" error code.
      code <- as.integer(code)
      if (code < 0 || code > 2^16 - 1) {
        warning("Invalid websocket error code: ", code)
        code <- 1001L
      }
      reason <- iconv(reason, to = "UTF-8")

      closeWS(.handle, code, reason)
      .handle <<- NULL
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

#' @details \code{startServer} binds the specified port and listens for
#'   connections on an thread running in the background. This background thread
#'   handles the I/O, and when it receives a HTTP request, it will schedule a
#'   call to the user-defined R functions in \code{app} to handle the request.
#'   This scheduling is done with \code{\link[later]{later}()}. When the R call
#'   stack is empty -- in other words, when an interactive R session is sitting
#'   idle at the command prompt -- R will automatically run the scheduled calls.
#'   However, if the call stack is not empty -- if R is evaluating other R code
#'   -- then the callbacks will not execute until either the call stack is
#'   empty, or the \code{\link[later]{run_now}()} function is called. This
#'   function tells R to execute any callbacks that have been scheduled by
#'   \code{\link[later]{later}()}. The \code{\link{service}()} function is
#'   essentially a wrapper for \code{\link[later]{run_now}()}.
#'
#'   In older versions of httpuv (1.3.5 and below), it did not use a background
#'   thread for I/O, and when this function was called, it did not accept
#'   connections immediately. It was necessary to call \code{\link{service}}
#'   repeatedly in order to actually accept and handle connections.
#'
#'   If the port cannot be bound (most likely due to permissions or because it
#'   is already bound), an error is raised.
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
#' @seealso \code{\link{stopServer}}, \code{\link{runServer}}
#' @aliases startPipeServer
#'
#' @examples
#' \dontrun{
#' # A very basic application
#' handle <- startServer("0.0.0.0", 5000,
#'   list(
#'     call = function(req) {
#'       list(
#'         status = 200L,
#'         headers = list(
#'           'Content-Type' = 'text/html'
#'         ),
#'         body = "Hello world!"
#'       )
#'     }
#'   )
#' )
#'
#' stopServer(handle)
#' }
#' @export
startServer <- function(host, port, app) {
  
  appWrapper <- AppWrapper$new(app)
  server <- makeTcpServer(
    host, port,
    appWrapper$onHeaders,
    appWrapper$onBodyData,
    appWrapper$call,
    appWrapper$onWSOpen,
    appWrapper$onWSMessage,
    appWrapper$onWSClose
  )

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
#' Process HTTP requests and WebSocket messages. If there is nothing on R's call
#' stack -- if R is sitting idle at the command prompt -- it is not necessary to
#' call this function, because requests will be handled automatically. However,
#' if R is executing code, then requests will not be handled until either the
#' call stack is empty, or this function is called (or alternatively,
#' \code{\link[later]{run_now}} is called).
#'
#' In previous versions of httpuv (1.3.5 and below), even if a server created by
#' \code{\link{startServer}} exists, no requests were serviced unless and until
#' \code{service} was called.
#'
#' This function simply calls \code{\link[later]{run_now}()}, so if your
#' application schedules any \code{\link[later]{later}} callbacks, they will be
#' invoked.
#'
#' @param timeoutMs Approximate number of milliseconds to run before returning.
#'   It will return this duration has elapsed. If 0 or Inf, then the function
#'   will continually process requests without returning unless an error occurs.
#'   If NA, performs a non-blocking run without waiting.
#'
#' @examples
#' \dontrun{
#' while (TRUE) {
#'   service()
#' }
#' }
#'
#' @export
#' @importFrom later run_now
service <- function(timeoutMs = ifelse(interactive(), 100, 1000)) {
  if (is.na(timeoutMs)) {
    # NA means to run non-blocking
    run_now(0)

  } else if (timeoutMs == 0 || timeoutMs == Inf) {
    .globals$paused <- FALSE
    # In interactive sessions, wait for a max of 0.1 seconds for better
    # responsiveness when the user sends an interrupt (like Esc in RStudio.)
    check_time <- if (interactive()) 0.1 else Inf
    while (!.globals$paused) {
      run_now(check_time)
    }

  } else {
    # No need to check for .globals$paused because if run_now() executes
    # anything, it will return immediately.
    run_now(timeoutMs / 1000)
  }

  # Some code expects service() to return TRUE (#123)
  TRUE
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
#' @param interruptIntervalMs Deprecated (last used in httpuv 1.3.5).
#'   
#' @seealso \code{\link{startServer}}, \code{\link{service}},
#'   \code{\link{stopServer}}
#'
#' @examples
#' \dontrun{
#' # A very basic application
#' runServer("0.0.0.0", 5000,
#'   list(
#'     call = function(req) {
#'       list(
#'         status = 200L,
#'         headers = list(
#'           'Content-Type' = 'text/html'
#'         ),
#'         body = "Hello world!"
#'       )
#'     }
#'   )
#' )
#' }
#' @export
runServer <- function(host, port, app, interruptIntervalMs = NULL) {
  server <- startServer(host, port, app)
  on.exit(stopServer(server))
  
  # TODO: in the future, add deprecation message to interruptIntervalMs.
  service(0)
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
  .globals$paused <- TRUE
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


#' Create an HTTP/WebSocket daemonized server (deprecated)
#'
#' This function will be removed in a future release of httpuv. It is simply a
#' wrapper for \code{\link{startServer}}. In previous versions of httpuv (1.3.5
#' and below), \code{startServer} ran applications in the foreground and
#' \code{startDaemonizedServer} ran applications in the background, but now both
#' of them run applications in the background.
#'
#' @inheritParams startServer
#' @export
startDaemonizedServer <- startServer

#' Stop a running daemonized server in Unix environments (deprecated)
#'
#' This function will be removed in a future release of httpuv. Instead, use
#' \code{\link{stopServer}}.
#'
#' @inheritParams stopServer
#'
#' @export
stopDaemonizedServer <- stopServer


# Needed so that Rcpp registers the 'httpuv_decodeURIComponent' symbol
legacy_dummy <- function(value){
  .Call('httpuv_decodeURIComponent', PACKAGE = "httpuv", value)
}
