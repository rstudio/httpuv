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

test_that("first endpoint", {
  port <- random_open_port()
  p <- process$new(
    "R",
    args = c("-e", paste0("app_port <- ", port, "; source('sample_app.R'); service(Inf)")),
    stdout = "|", stderr = "|", supervise = TRUE
  )
  Sys.sleep(1)
  outfile <- tempfile()
  bench <- process$new(
    "ab",
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/", port)),
    stdout = outfile, stderr = outfile
  )
  bench$wait(timeout = 20000)
  
  output_text <- readLines(outfile)
  results <- parse_ab_output(output_text)
  
  expect_false(results$hang)
  expect_equal(results$completed, 400)
  
  p$kill()
  bench$kill()
})

# Two concurrent tests - runs fine
test_that("Simple endpoint", {
  
  
  port <- random_open_port()
  p <- process$new(
    "R",
    args = c("-e", paste0("app_port <- ", port, "; source('sample_app.R'); service(Inf)")),
    stdout = "|", stderr = "|", supervise = TRUE
  )
  Sys.sleep(1)
  expect_true(p$is_alive())
  
  p$is_alive()
  
  outfile <- tempfile()
  bench <- process$new(
    "ab",
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/", port)),
    stdout = outfile, stderr = outfile
  )
  
  outfilea <- tempfile()
  bencha <- process$new(
    "ab",
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/", port)),
    stdout = outfilea, stderr = outfilea
  )
  
  
  readLines(outfile)
  readLines(outfilea)
  
  bench$wait(timeout = 20000)
  bencha$wait(timeout = 20000)
  
  readLines(outfile)
  readLines(outfilea)
  output_text <- readLines(outfile)
  results <- parse_ab_output(output_text)
  
  
  output_text
  
  expect_false(results$hang)
  expect_equal(results$completed, 400)
  
  output_texta <- readLines(outfilea)
  resultsa <- parse_ab_output(output_texta)
  
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 400)
  
  p$kill()
  bench$kill()
  bencha$kill()
  
})


test_that("header sync endpoint", {
  
  Sys.sleep(10)
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
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/header", port)),
    stdout = outfile, stderr = outfile
  )
  
  outfilea <- tempfile()
  bencha <- process$new(
    "ab",
    args = c("-n600", "-c100", sprintf("http://127.0.0.1:%d/sync", port)),
    stdout = outfilea, stderr = outfilea
  )
  
  bench$wait(timeout = 40000)
  bencha$wait(timeout = 40000)
  
  readLines(outfile)
  readLines(outfilea)
  
  output_text <- readLines(outfile)
  results <- parse_ab_output(output_text)
  
  expect_false(results$hang)
  expect_equal(results$completed, 400)
  
  output_texta <- readLines(outfilea)
  resultsa <- parse_ab_output(output_texta)

  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 600)
  
  p$kill()
  bench$kill()
  bencha$kill()
  
})

test_that("header async endpoint", {
  
  Sys.sleep(10)
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
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/header", port)),
    stdout = outfile, stderr = outfile
  )
  
  outfilea <- tempfile()
  bencha <- process$new(
    "ab",
    args = c("-n600", "-c100", sprintf("http://127.0.0.1:%d/async", port)),
    stdout = outfilea, stderr = outfilea
  )
  
  bench$wait(timeout = 40000)
  bencha$wait(timeout = 40000)
  
  readLines(outfile)
  readLines(outfilea)
  
  output_text <- readLines(outfile)
  results <- parse_ab_output(output_text)
  
  expect_false(results$hang)
  expect_equal(results$completed, 400)
  
  output_texta <- readLines(outfilea)
  resultsa <- parse_ab_output(output_texta)
  
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 600)
  
  p$kill()
  bench$kill()
  bencha$kill()
  
})

test_that("header async error endpoint", {
  
  Sys.sleep(10)
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
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/header", port)),
    stdout = outfile, stderr = outfile
  )
  
  outfilea <- tempfile()
  bencha <- process$new(
    "ab",
    args = c("-n600", "-c100", sprintf("http://127.0.0.1:%d/async-error", port)),
    stdout = outfilea, stderr = outfilea
  )
  
  bench$wait(timeout = 40000)
  bencha$wait(timeout = 40000)
  
  readLines(outfile)
  readLines(outfilea)
  
  output_text <- readLines(outfile)
  results <- parse_ab_output(output_text)
  
  expect_false(results$hang)
  expect_equal(results$completed, 400)
  
  output_texta <- readLines(outfilea)
  resultsa <- parse_ab_output(output_texta)
  
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 600)
  
  p$kill()
  bench$kill()
  bencha$kill()
  
})

test_that("header sync error endpoint", {
  
  Sys.sleep(10)
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
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/header", port)),
    stdout = outfile, stderr = outfile
  )
  
  outfilea <- tempfile()
  bencha <- process$new(
    "ab",
    args = c("-n600", "-c100", sprintf("http://127.0.0.1:%d/sync-error", port)),
    stdout = outfilea, stderr = outfilea
  )
  
  bench$wait(timeout = 40000)
  bencha$wait(timeout = 40000)
  
  readLines(outfile)
  readLines(outfilea)
  
  output_text <- readLines(outfile)
  results <- parse_ab_output(output_text)
  
  expect_false(results$hang)
  expect_equal(results$completed, 400)
  
  output_texta <- readLines(outfilea)
  resultsa <- parse_ab_output(output_texta)
  
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 600)
  
  p$kill()
  bench$kill()
  bencha$kill()
  
})

test_that("async async error endpoint", {
  
  Sys.sleep(10)
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
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/async", port)),
    stdout = outfile, stderr = outfile
  )
  
  outfilea <- tempfile()
  bencha <- process$new(
    "ab",
    args = c("-n600", "-c100", sprintf("http://127.0.0.1:%d/async-error", port)),
    stdout = outfilea, stderr = outfilea
  )
  
  bench$wait(timeout = 40000)
  bencha$wait(timeout = 40000)
  
  readLines(outfile)
  readLines(outfilea)
  
  output_text <- readLines(outfile)
  results <- parse_ab_output(output_text)
  
  expect_false(results$hang)
  expect_equal(results$completed, 400)
  
  output_texta <- readLines(outfilea)
  resultsa <- parse_ab_output(output_texta)
  
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 600)
  
  p$kill()
  bench$kill()
  bencha$kill()
  
})



test_that("body error async error endpoint", {
  
  Sys.sleep(10)
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
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/body-error", port)),
    stdout = outfile, stderr = outfile
  )
  
  outfilea <- tempfile()
  bencha <- process$new(
    "ab",
    args = c("-n600", "-c100", sprintf("http://127.0.0.1:%d/async-error", port)),
    stdout = outfilea, stderr = outfilea
  )
  
  bench$wait(timeout = 40000)
  bencha$wait(timeout = 40000)
  
  readLines(outfile)
  readLines(outfilea)
  
  output_text <- readLines(outfile)
  results <- parse_ab_output(output_text)
  
  expect_false(results$hang)
  expect_equal(results$completed, 400)
  
  output_texta <- readLines(outfilea)
  resultsa <- parse_ab_output(output_texta)
  
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 600)
  
  p$kill()
  bench$kill()
  bencha$kill()
  
})

#Check this Failing, header delay is very slow
test_that("header error header print endpoint", {
  
  Sys.sleep(10)
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
    args = c("-n400", "-c100", sprintf("http://127.0.0.1:%d/header-error", port)),
    stdout = outfile, stderr = outfile
  )
  
  outfilea <- tempfile()
  bencha <- process$new(
    "ab",
    args = c("-n400", "-c10", sprintf("http://127.0.0.1:%d/header-print", port)),
    stdout = outfilea, stderr = outfilea
  )
  
  bench$wait(timeout = 40000)
  bencha$wait(timeout = 40000)
  
  readLines(outfile)
  readLines(outfilea)
  
  output_text <- readLines(outfile)
  
  results <- parse_ab_output(output_text)
  
  expect_false(results$hang)
  expect_equal(results$completed, 400)
  
  output_texta <- readLines(outfilea)
  
  resultsa <- parse_ab_output(output_texta)
  
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 6)
  
  p$kill()
  bench$kill()
  bencha$kill()
  
})




