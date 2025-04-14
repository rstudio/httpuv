path_example_site <- function(...) {
  system.file("example-static-site", ..., package = "httpuv")
}

index_file_content <- raw_file_content(path_example_site("index.html"))
office_file_content <- raw_file_content(path_example_site("office.html"))

expect_example_site <- function(port, host = "127.0.0.1") {
  res <- fetch(local_url("/index.html", port), gzip = FALSE)
  expect_equal(res$status_code, 200)
  expect_identical(res$content, index_file_content)

  res <- fetch(local_url("/office.html", port), gzip = FALSE)
  expect_equal(res$status_code, 200)
  expect_identical(res$content, office_file_content)
}

start_example_server <- function(port) {
  r <- callr::r_bg(
    function(port) {
      ex <- system.file("example-static-site", package = "httpuv")
      httpuv::runStaticServer(ex, port = port, background = FALSE, browse = FALSE)
    },
    list(port = port)
  )

  max <- Sys.time() + 2
  while(length(r$read_error_lines()) == 0) {
    if (Sys.time() > max) {
      skip("Server didn't start up in 2 seconds")
    }
    Sys.sleep(0.1)
  }

  r
}

test_that("runStaticServer() in foreground with custom port", {
  port <- randomPort()

  r <- start_example_server(port)
  on.exit({ r$kill() }, add = TRUE)

  expect_example_site(port)
})

test_that("runStaticServer() in foreground with default port", {
  skip_if_not(is_port_available(7446))

  r <- start_example_server(NULL)
  on.exit({ r$kill() }, add = TRUE)

  expect_example_site(7446)
})

test_that("runStaticServer() throws an error for invalid ports", {
  on.exit({ stopAllServers() }) # in case of a test failure

  expect_error(
    runStaticServer(path_example_site(), port = 0, background = TRUE)
  )
  expect_error(
    runStaticServer(path_example_site(), port = 700:720, background = TRUE)
  )
  expect_error(
    runStaticServer(path_example_site(), port = 74469, background = TRUE)
  )
  expect_error(
    runStaticServer(path_example_site(), port = "1234", background = TRUE)
  )
})

test_that("runStaticServer() throws an error if the requested port is used", {
  on.exit({ stopAllServers() }) # in case of a test failure

  s1 <- runStaticServer(path_example_site(), background = TRUE, browse = FALSE)

  expect_error(
    runStaticServer(path_example_site(), port = s1$getPort(), background = TRUE, browse = FALSE)
  )
})

test_that("runStaticServer() in background uses default port", {
  skip_if_not(is_port_available(7446))

  s <- runStaticServer(path_example_site(), background = TRUE, browse = FALSE)
  on.exit({ stopServer(s) }, add = TRUE)

  expect_example_site(7446)
})

test_that("runStaticServer() in background uses default port or random port", {
  skip_if_not(is_port_available(7446))

  s1 <- runStaticServer(path_example_site(), background = TRUE, browse = FALSE)
  on.exit({ s1$stop() }, add = TRUE)

  s2 <- runStaticServer(path_example_site(), background = TRUE, browse = FALSE)
  on.exit({ s2$stop() }, add = TRUE)

  expect_example_site(7446)
  expect_example_site(s2$getPort())
})

test_that("runStaticServer() in background errors if requested port is in use", {
  s1 <- runStaticServer(path_example_site(), background = TRUE, browse = FALSE)
  on.exit({ s1$stop() }, add = TRUE)

  used_port <- s1$getPort()

  expect_error({
    s2 <- runStaticServer(path_example_site(), port = used_port, background = TRUE, browse = FALSE)
    s2$stop() # clean up in case test fails
  })
})

test_that("runStaticServer() prints informative console messages", {
  local_edition(3)

  expect_snapshot({
    s <- runStaticServer(path_example_site(), background = TRUE, browse = FALSE)
    s$stop()
  }, transform = function(x) {
    sub(path_example_site(), "/Users/user/path/to/site", x, fixed = TRUE)
  })
})
