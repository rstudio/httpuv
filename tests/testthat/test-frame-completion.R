# Regression test of
# https://github.com/rstudio/httpuv/pull/219

context("frame completion")

test_that("a close message with no payload is processed", {
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

  on.exit(srv$stop())

  # "Unnecessary" braces here to prevent `later` from attempting to
  # run callbacks if this test is pasted at the console
  {
    ws_client <- websocket::WebSocket$new(sprintf("ws://127.0.0.1:%s", random_port))
    ws_client$onOpen(function(event) {
      ws_client$close(NA)
    })
  }

  loop_start <- Sys.time()
  while (!later::loop_empty()) {
    if ((Sys.time() - loop_start) > as.difftime(10, units = "secs")) stop("run loop timed out")
    later::run_now()
  }

  testthat::expect_true(elapsed < as.difftime(1, units = "secs"))
})
