#' @useDynLib eventloop
NULL

AppWrapper <- setRefClass(
  'AppWrapper',
  fields = list(
    .app = 'ANY',
    .wsconns = 'environment'
  ),
  methods = list(
    initialize = function(app) {
      .app <<- app
    },
    call = function(req) {
      result <- try({
        resp <- .app$call(req)
        if ('file' %in% names(resp$body)) {
          filename <- resp$body[['file']]
          resp$body <- readBin(filename, raw(), file.info(filename)$size)
        }
        resp
      })
      if (inherits(result, 'try-error')) {
        return(list(
          status=500L,
          headers=list(
            'Content-Type'='text/plain'
          ),
          body=charToRaw(
            paste("ERROR:", attr(result, "condition")$message, collapse="\n"))
        ))
      } else {
        return(result)
      }
    },
    onWSOpen = function(handle) {
      ws <- WebSocket$new(handle)
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

#' @export
WebSocket <- setRefClass(
  'WebSocket',
  fields = list(
    '.handle' = 'ANY',
    '.messageCallbacks' = 'list',
    '.closeCallbacks' = 'list'
  ),
  methods = list(
    initialize = function(handle) {
      .handle <<- handle
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

#' @export
run <- function(host, port, app) {
  if (server != 0) {
    on.exit(destroyServer(server))
    while (runNB()) {
      Sys.sleep(0.01)
    }
  }
}

#' @export
createServer <- function(host, port, app, maxTimeout) {
  
  appWrapper <- AppWrapper$new(app)
  server <- makeServer(host, port, maxTimeout,
                       appWrapper$call,
                       appWrapper$onWSOpen,
                       appWrapper$onWSMessage,
                       appWrapper$onWSClose)
  if (is.null(server)) {
    stop("Failed to create server")
  }
  return(server)
}

#' @export
service <- function() {
  runOnce()
}

#' @export
stopServer <- function(handle) {
  destroyServer(handle)
}
