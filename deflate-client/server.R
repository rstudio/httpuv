library(httpuv)


echo_server <- function(ws) {
  ws$onMessage(function(isBinary, data) {
    ws$send(data)
  })
}

md5_server <- function(ws) {
  ws$onMessage(function(isBinary, data) {
    ws$send(digest::digest(data, serialize = FALSE))
  })
}

server <- list(
  onWSOpen = function(ws) {
    if (identical(ws$request$PATH_INFO, "/echo")) {
      echo_server(ws)
    } else if (identical(ws$request$PATH_INFO, "/md5")) {
      md5_server(ws)
    } else {
      ws$close()
    }
  }
)

httpuv::runServer("127.0.0.1", 9100, server)