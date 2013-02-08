(function() {
  require(eventloop)
  server <- makeServer(function(env) {
    print(as.list(env))
    paste(as.character(rnorm(10)), collapse='\n')
  })
  if (server != 0) {
    on.exit(destroyServer(server))
    while (run()) {
      Sys.sleep(0.01)
    }
  }
})()