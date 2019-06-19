# Regression test of
# https://github.com/rstudio/httpuv/pull/219

context("frame completion")

elapsed <- NULL

random_port <- random_open_port()

srv <- startServer("127.0.0.1", random_port, list(
  onWSOpen = function(ws) {
    open_time <- Sys.time()
    ws$onClose(function(e) {
      elapsed <<- Sys.time() - open_time
      stopServer(srv)
    })
  }
))

# "Unnecessary" braces here to prevent `later` from attempting to
# run callbacks if this test is pasted at the console
{
  ws_client <- websocket::WebSocket$new(sprintf("ws://127.0.0.1:%s", random_port))
  ws_client$onOpen(function(event) {
    ws_client$close(NA)
  })
}

while (!later::loop_empty()) {
  later::run_now()
}

testthat::expect_true(elapsed < as.difftime(0.2, units = "secs"))
