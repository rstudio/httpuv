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
    onWSOpen = function(handle) {
      ws <- WebSocket$new(handle)
      .wsconns[[as.character(handle)]] <<- ws
      .app$onWSOpen(ws)
    },
    onWSMessage = function(handle, binary, message) {
      for (handler in .wsconns[[as.character(handle)]]$.messageCallbacks) {
        handler(binary, message)
      }
    },
    onWSClose = function(handle) {
      ws <- .wsconns[[as.character(handle)]]
      ws$.handle <- NULL
      print(paste("handle:", handle))
      rm(list=as.character(handle), pos=.wsconns)
      print(paste("handle1:", handle))
      for (handler in ws$.closeCallbacks) {
        handler()
      }
    }
  )
)

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
    }
  )
)

run <- function(host, port, app) {
  appWrapper <- AppWrapper$new(app)
  server <- makeServer(host, port, app$call,
                       appWrapper$onWSOpen,
                       appWrapper$onWSMessage,
                       appWrapper$onWSClose)
  if (server != 0) {
    on.exit(destroyServer(server))
    while (runNB()) {
      Sys.sleep(0.01)
    }
  }
}
