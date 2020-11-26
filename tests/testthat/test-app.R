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

  r1 <- fetch(local_url("/", s1$getPort()))
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
