# runStaticServer() prints informative console messages

    Code
      s <- runStaticServer(path_example_site(), background = TRUE, browse = FALSE)
    Message
      Serving: '/Users/user/path/to/site'
      View at: http://127.0.0.1:7446
    Code
      s$stop()

