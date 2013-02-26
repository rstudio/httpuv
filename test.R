library(httpuv)

(function() {
  require(httpuv)
  run("0.0.0.0", 8002, list(
    call = function(env) {
      #print(as.list(env))
      list(
        status=200L,
        headers=list(
          #'Content-Type'='text/html'
          'Content-Type'='application/pdf'
        ),
        #body="<strong>Hello</strong> <em>world</em>"
        #body=charToRaw(paste(as.character(rnorm(10)), collapse='\n'))
        body=c(file='~/Downloads/10.1.1.23.360.pdf')
      )
    },
    onWSOpen = function(ws) {
      print("Got WS!")
      ws$send("hello")
      
      ws$onMessage(function(binary, msg) {
        cat("Message received!\n")
        #cat(paste('"', msg, '"', sep=''))
        cat("\n")
        ws$send(msg)
        #ws$close()
      })
      ws$onClose(function() {
        cat("WSClose\n");
      })
    }
  ))
})()