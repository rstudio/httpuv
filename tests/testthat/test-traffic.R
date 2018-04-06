context("traffic")

skip_if_not_possible <- function() {
  # Temporarily disable these tests because they may not run reliably on
  # some platforms.
  skip("")
  # skip_on_cran()

  if (Sys.which("ab")[[1]] == "") {
    skip("ab (Apache bench) not available for running traffic tests")
  }
}

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


parse_ab_output <- function(p) {
  text <- readLines(p$get_output_file())

  if (!any(grepl("^Complete requests:\\s+", text))) {
    return(list(
      completed = NULL,
      failed = NULL,
      hang = TRUE,
      text = text
    ))
  }

  results <- list(
    hang = FALSE,
    text = text
  )

  line <- text[grepl("^Complete requests:\\s+", text)]
  results$completed <- as.integer(sub("^Complete requests:\\s+(\\d+).*", "\\1", line))

  line <- text[grepl("^Failed requests:\\s+", text)]
  results$failed <- as.integer(sub("^Failed requests:\\s+(\\d+).*", "\\1", line))

  results
}

# Launch sample_app process and return process object
start_app <- function(port) {
  outfile <- tempfile()
  callr::process$new(
    "R",
    args = c("-e", paste0("app_port <- ", port, "; source('sample_app.R'); service(Inf)")),
    stdout = outfile, stderr = outfile, supervise = TRUE
  )
}

# Start an apachebench test
start_ab <- function(port, path, n = 400, concurrent = 100) {
  outfile <- tempfile()
  callr::process$new(
    "ab",
    args = c(paste0("-n", n), paste0("-c", concurrent), sprintf("http://127.0.0.1:%d%s", port, path)),
    stdout = outfile, stderr = outfile
  )
}


test_that("Basic traffic test", {
  skip_if_not_possible()
  port <- random_open_port()
  p <- start_app(port)
  Sys.sleep(1)

  bench <- start_ab(port, "/")
  bench$wait(20000)

  results <- parse_ab_output(bench)
  expect_false(results$hang)
  expect_equal(results$completed, 400)

  p$kill()
  bench$kill()
})

test_that("Two concurrent", {
  skip_if_not_possible()
  port <- random_open_port()
  p <- start_app(port)
  Sys.sleep(1)
  expect_true(p$is_alive())

  bench <- start_ab(port, "/")
  bencha <- start_ab(port, "/")

  bench$wait(20000)
  bencha$wait(20000)

  results <- parse_ab_output(bench)
  expect_false(results$hang)
  expect_equal(results$completed, 400)

  resultsa <- parse_ab_output(bencha)
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 400)

  p$kill()
  bench$kill()
  bencha$kill()
})


test_that("/header /sync endpoints", {
  skip_if_not_possible()
  port <- random_open_port()
  p <- start_app(port)

  Sys.sleep(1)
  expect_true(p$is_alive())

  bench <- start_ab(port, "/header")
  bencha <- start_ab(port, "/sync")

  bench$wait(40000)
  bencha$wait(40000)

  results <- parse_ab_output(bench)
  expect_false(results$hang)
  expect_equal(results$completed, 400)

  resultsa <- parse_ab_output(bencha)
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 400)

  p$kill()
  bench$kill()
  bencha$kill()
})

test_that("/header /async endpoints", {
  skip_if_not_possible()
  port <- random_open_port()
  p <- start_app(port)

  Sys.sleep(1)
  expect_true(p$is_alive())

  bench <- start_ab(port, "/header")
  bencha <- start_ab(port, "/async")

  bench$wait(40000)
  bencha$wait(40000)

  results <- parse_ab_output(bench)
  expect_false(results$hang)
  expect_equal(results$completed, 400)

  resultsa <- parse_ab_output(bencha)
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 400)

  p$kill()
  bench$kill()
  bencha$kill()
})


test_that("/header /async-error endpoints", {
  skip_if_not_possible()
  port <- random_open_port()
  p <- start_app(port)

  Sys.sleep(1)
  expect_true(p$is_alive())

  bench <- start_ab(port, "/header")
  bencha <- start_ab(port, "/async-error")

  bench$wait(40000)
  bencha$wait(40000)

  results <- parse_ab_output(bench)
  expect_false(results$hang)
  expect_equal(results$completed, 400)

  resultsa <- parse_ab_output(bencha)
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 400)

  p$kill()
  bench$kill()
  bencha$kill()
})


test_that("/async /async-error endpoints", {
  skip_if_not_possible()
  port <- random_open_port()
  p <- start_app(port)

  Sys.sleep(1)
  expect_true(p$is_alive())

  bench <- start_ab(port, "/async")
  bencha <- start_ab(port, "/async-error")

  bench$wait(40000)
  bencha$wait(40000)

  results <- parse_ab_output(bench)
  expect_false(results$hang)
  expect_equal(results$completed, 400)

  resultsa <- parse_ab_output(bencha)
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 400)

  p$kill()
  bench$kill()
  bencha$kill()
})



test_that("/body-error /async-error endpoints", {
  skip_if_not_possible()
  port <- random_open_port()
  p <- start_app(port)

  Sys.sleep(1)
  expect_true(p$is_alive())

  bench <- start_ab(port, "/body-error")
  bencha <- start_ab(port, "/async-error")

  bench$wait(40000)
  bencha$wait(40000)

  results <- parse_ab_output(bench)
  expect_false(results$hang)
  expect_equal(results$completed, 400)

  resultsa <- parse_ab_output(bencha)
  expect_false(resultsa$hang)
  expect_equal(resultsa$completed, 400)

  p$kill()
  bench$kill()
  bencha$kill()
})
