context("traffic")

library(processx)

random_open_port <- function(min = 3000, max = 9000, n = 20) {
  # Unsafe port list from shiny::runApp()
  valid_ports <- setdiff(min:max, c(3659, 4045, 6000, 6665:6669, 6697))

  # Try up to n ports
  for (port in sample(valid_ports, n)) {
    handle <- NULL

    # Check if port is open
    tryCatch(
      handle <- httpuv::startServer("127.0.0.1", port, list()),
      error = function(e) { }
    )
    if (!is.null(handle)) {
      httpuv::stopServer(handle)
      return(port)
    }
  }

  stop("Cannot find an available port")
}


parse_ab_output <- function(txt) {
  if (!any(grepl("^Complete requests:\\s+", txt))) {
    return(list(
      completed = NULL,
      failed = NULL,
      hang = TRUE
    ))
  }

  results <- list(
    hang = FALSE
  )

  line <- txt[grepl("^Complete requests:\\s+", txt)]
  results$completed <- as.integer(sub("^Complete requests:\\s+(\\d+).*", "\\1", line))

  line <- txt[grepl("^Failed requests:\\s+", txt)]
  results$failed <- as.integer(sub("^Failed requests:\\s+(\\d+).*", "\\1", line))

  results
}


test_that("sync endpoint", {
  port <- random_open_port()
  p <- process$new(
    "R",
    args = c("-e", paste0("app_port <- ", port, "; source('sample_app.R'); service(Inf)")),
    stdout = "|", stderr = "|", supervise = TRUE
  )
  Sys.sleep(1)
  expect_true(p$is_alive())

  outfile <- tempfile()
  bench <- process$new(
    "ab",
    args = c("-n4000", "-c100", sprintf("http://127.0.0.1:%d/", port)),
    stdout = outfile, stderr = outfile
  )
  bench$wait(timeout = 20000)
  
  output_text <- readLines(outfile)
  results <- parse_ab_output(output_text)

  expect_false(results$hang)
  expect_equal(results$completed, 4000)
  
  p$kill()
  bench$kill()
})


test_that("async endpoint", {
})

