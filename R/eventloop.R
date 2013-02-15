run <- function(host, port, app) {
  server <- makeServer(host, port, app$call, app$onWSOpen, app$onWSMessage, app$onWSClose)
  if (server != 0) {
    on.exit(destroyServer(server))
    while (runNB()) {
      Sys.sleep(0.01)
    }
  }
}
