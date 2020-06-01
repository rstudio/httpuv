context("http-parse")

test_that("Large HTTP header values are preserved", {
  # This is a test for https://github.com/rstudio/httpuv/issues/275
  # When there is a very large header, it may span multiple TCP messages.
  # Previously, these headers would get truncated.
  s <- httpuv::startServer("0.0.0.0", randomPort(),
    list(
      call = function(req) {
        list(
          status = 200L,
          headers = list('Content-Type' = 'text/plain'),
          # Use paste0("", ...) in case the header is NULL
          body = paste0("", req$HTTP_TEST_HEADER)
        )
      }
    )
  )
  on.exit(s$stop())

  # Create a request with a large header (80000 bytes)
  # The maximum size of a TCP message is 64kB, so I believe this header will
  # necessarily result in more than 1 call to HttpRequest::_on_header_value().
  # Note that this is under the HTTP_MAX_HEADER_SIZE defined in http_parser.h
  # to be 80*1024. A larger message would result in the server just closing the
  # connection.
  long_string <- paste0(rep(".", 80000), collapse = "")
  h <- new_handle()
  handle_setheaders(h, `test-header` = long_string)

  res <- fetch(local_url("/", s$getPort()),  h)
  content <- rawToChar(res$content)
  expect_identical(content, long_string)


  # Similar to previous, but make sure there are two header entries with the
  # same field name, as in:
  #   foo: aaaaaaaa....
  #   foo: bbbbbbbb....
  # The resulting value of foo should should be "aaaaaa,bbbbbbbbbbbb"
  # (Note: I've tested, and repeating the same header name with curl does result
  # two of those headers.)
  long_string_a <- paste0(rep("a", 100), collapse = "")
  long_string_b <- paste0(rep("b", 80000), collapse = "")

  # The second test-header value is the long one, so it will be split across
  # multiple TCP messages.
  h <- new_handle()
  handle_setheaders(h, `test-header` = long_string_a, `test-header` = long_string_b)
  res <- fetch(local_url("/", s$getPort()),  h)
  content <- rawToChar(res$content)
  expect_identical(content, paste0(long_string_a, ",", long_string_b))

  # The first test-header value is the long one, so it will be split across
  # multiple TCP messages.
  h <- new_handle()
  handle_setheaders(h, `test-header` = long_string_b, `test-header` = long_string_a)
  res <- fetch(local_url("/", s$getPort()),  h)
  content <- rawToChar(res$content)
  expect_identical(content, paste0(long_string_b, ",", long_string_a))
})


test_that("Large HTTP header field names are preserved", {
  # Also for https://github.com/rstudio/httpuv/issues/275
  # This tests for field names that are split across messages.
  headers_received <- NULL
  s <- httpuv::startServer("0.0.0.0", randomPort(),
    list(
      call = function(req) {
        # Save the headers for examination later
        headers_received <<- req$HEADERS
        list(
          status = 200L,
          headers = list('Content-Type' = 'text/plain'),
          body = paste0("OK")
        )
      }
    )
  )
  on.exit(s$stop())
  # Test for long field names, as in:
  #  aaaaaa...aaaaaa: A
  #  bbbbbb...bbbbbb: B
  # Variable names in R must be 10000 bytes or less, so we need several of them
  # to do this test.
  h <- new_handle()
  values <- as.list(LETTERS[1:8])
  # Use 9900-byte field names (instead of 10000) because the Rook object makes
  # them longer by prepending "HTTP_".
  fields <- vapply(letters[1:8], function(x) paste0(rep(x, 9900), collapse = ""), "")
  headers <- setNames(values, fields)
  do.call(handle_setheaders, c(list(h), headers))

  res <- fetch(local_url("/", s$getPort()),  h)
  expect_true(all(fields %in% names(headers_received)))
  expect_identical(as.list(headers_received[fields]), headers)
})
