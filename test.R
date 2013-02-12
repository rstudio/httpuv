library(eventloop)
(function() {
  require(eventloop)
  run("0.0.0.0", 8002, function(env) {
    #print(as.list(env))
    charToRaw(paste(as.character(rnorm(10)), collapse='\n'))
  })
})()