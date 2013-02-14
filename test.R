library(eventloop)

(function() {
  require(eventloop)
  run("0.0.0.0", 8002, list(
    call = function(env) {
      #print(as.list(env))
      charToRaw(paste(as.character(rnorm(10)), collapse='\n'))
    },
    onWSOpen = function(ws) {
      
    },
    onWSMessage = function(ws, binary, msg) {
      cat("Message received!\n")
      cat(paste('"', msg, '"', sep=''))
      cat("\n")
    },
    onWSClose = function() {
      cat("WSClose\n");
    }
  ))
})()