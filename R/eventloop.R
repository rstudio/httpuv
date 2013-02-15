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
      .messageCallbacks[[length(.messageCallbacks)]] <<- func
    },
    onClose = function(func) {
      .closeCallbacks[[length(.closeCallbacks)]] <<- func
    },
    send = function(message) {
      
    }
  )
)

run <- function(host, port, app) {
  server <- makeServer(host, port, app$call, app$onWSOpen, app$onWSMessage, app$onWSClose)
  if (server != 0) {
    on.exit(destroyServer(server))
    while (runNB()) {
      Sys.sleep(0.01)
    }
  }
}
