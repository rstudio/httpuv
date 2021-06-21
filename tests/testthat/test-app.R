context("basic")

test_that("Basic functionality", {
  s1 <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        list(
          status = 200L,
          headers = list('Content-Type' = 'text/html'),
          body = "server 1"
        )
      }
    )
  )
  expect_equal(length(listServers()), 1)

  s2 <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        list(
          status = 200L,
          headers = list('Content-Type' = 'text/html'),
          body = "server 2"
        )
      }
    )
  )
  expect_equal(length(listServers()), 2)

  r1 <- fetch(local_url("/", s1$getPort()), gzip = FALSE)
  r2 <- fetch(local_url("/", s2$getPort()))

  expect_equal(r1$status_code, 200)
  expect_equal(r2$status_code, 200)

  expect_identical(rawToChar(r1$content), "server 1")
  expect_identical(rawToChar(r2$content), "server 2")

  expect_identical(parse_headers_list(r1$headers)$`content-type`, "text/html")
  expect_identical(parse_headers_list(r1$headers)$`content-length`, "8")

  s1$stop()
  expect_equal(length(listServers()), 1)
  stopAllServers()
  expect_equal(length(listServers()), 0)
})


test_that("Empty and NULL headers are OK", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        if (req$PATH_INFO == "/null") {
          list(
            status = 200L,
            headers = NULL,
            body = ""
          )
        } else if (req$PATH_INFO == "/emptylist") {
          list(
            status = 200L,
            headers = list(),
            body = ""
          )
        } else if (req$PATH_INFO == "/noheaders") {
          list(
            status = 200L,
            body = ""
          )
        }
      }
    )
  )
  on.exit(s$stop())
  expect_equal(length(listServers()), 1)

  r <- fetch(local_url("/null", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, raw())

  r <- fetch(local_url("/emptylist", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, raw())

  r <- fetch(local_url("/noheaders", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, raw())
})


test_that("Content length depends on the presence of 'body'", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        if (req$PATH_INFO == "/ok") {
          list(
            status = 200L,
            headers = list('Content-Type' = 'text/html'),
            body = if (req$REQUEST_METHOD != "HEAD") raw()
          )
        } else if (req$PATH_INFO == "/nullbody") {
          list(
            status = 204L,
            headers = list('Content-Type' = 'text/html'),
            body = NULL
          )
        } else if (req$PATH_INFO == "/nobody") {
          list(
            status = 204L,
            headers = list('Content-Type' = 'text/html')
          )
        }
      }
    )
  )
  on.exit(s$stop())
  expect_equal(length(listServers()), 1)

  r1 <- fetch(local_url("/ok", s$getPort()), gzip = FALSE)
  # HEAD requests should not have a body.
  r2 <- fetch(local_url("/ok", s$getPort()), new_handle(nobody = TRUE))
  r3 <- fetch(local_url("/nullbody", s$getPort()))
  r4 <- fetch(local_url("/nobody", s$getPort()))

  expect_equal(r1$status_code, 200)
  expect_equal(r2$status_code, 200)
  expect_equal(r3$status_code, 204)
  expect_equal(r3$status_code, 204)

  expect_equal(length(r1$content), 0)
  expect_equal(length(r2$content), 0)
  expect_equal(length(r3$content), 0)
  expect_equal(length(r4$content), 0)

  expect_identical(parse_headers_list(r1$headers)$`content-length`, "0")
  # HEAD requests *can* have a content-length, but they don't have to.
  expect_identical(parse_headers_list(r2$headers)$`content-length`, NULL)
  expect_identical(parse_headers_list(r3$headers)$`content-length`, NULL)
  expect_identical(parse_headers_list(r4$headers)$`content-length`, NULL)
})
