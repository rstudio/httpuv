(function() {
  require(eventloop)
  server <- makeServer(function(env) {
    print(c(method=env$REQUEST_METHOD, url=env$URL))
    paste(as.character(rnorm(10)), collapse='\n')
  })
  if (server != 0) {
    on.exit(destroyServer(server))
    while (run()) {
      Sys.sleep(0.01)
    }
  }
})()