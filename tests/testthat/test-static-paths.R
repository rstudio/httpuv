context("static")

index_file_content <- raw_file_content(test_path("apps/content/index.html"))
data_file_content <- raw_file_content(test_path("apps/content/data.txt"))
subdir_index_file_content <- raw_file_content(test_path("apps/content/subdir/index.html"))
index_file_1_content <- raw_file_content(test_path("apps/content_1/index.html"))

test_that("Basic static file serving", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      staticPaths = list(
        # Testing out various leading and trailing slashes
        "/" = test_path("apps/content"),
        "/1" = test_path("apps/content"),
        "/2/" = test_path("apps/content/"),
        "3" = test_path("apps/content"),
        "4/" = test_path("apps/content/")
      ),
      staticPathOptions = staticPathOptions(
        headers = list("Test-Code-Path" = "C++")
      )
    )
  )
  on.exit(s$stop())

  # Fetch index.html
  r <- fetch(local_url("/", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, index_file_content)

  # index.html for subdirectory
  r_subdir <- fetch(local_url("/subdir", s$getPort()))
  expect_equal(r_subdir$status_code, 200)
  expect_identical(r_subdir$content, subdir_index_file_content)

  h <- parse_headers_list(r$headers)
  expect_equal(as.integer(h$`content-length`), length(index_file_content))
  expect_equal(as.integer(h$`content-length`), length(r$content))
  expect_identical(h$`content-type`, "text/html; charset=utf-8")
  expect_identical(h$`test-code-path`, "C++")
  # Check that response time is within 1 minute of now. (Possible DST problems?)
  expect_true(abs(as.numeric(parse_http_date(h$date)) - as.numeric(Sys.time())) < 60)


  # Testing index for other paths
  r1 <- fetch(local_url("/1", s$getPort()))
  h1 <- parse_headers_list(r1$headers)
  expect_identical(r$content, r1$content)
  expect_identical(h$`content-length`, h1$`content-length`)
  expect_identical(h$`content-type`, h1$`content-type`)

  r2 <- fetch(local_url("/1/", s$getPort()))
  h2 <- parse_headers_list(r2$headers)
  expect_identical(r$content, r2$content)
  expect_identical(h$`content-length`, h2$`content-length`)
  expect_identical(h$`content-type`, h2$`content-type`)

  r3 <- fetch(local_url("/1/index.html", s$getPort()))
  h3 <- parse_headers_list(r3$headers)
  expect_identical(r$content, r3$content)
  expect_identical(h$`content-length`, h3$`content-length`)
  expect_identical(h$`content-type`, h3$`content-type`)

  # Missing file (404)
  r <- fetch(local_url("/foo", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 404)
  expect_identical(rawToChar(r$content), "404 Not Found\n")
  expect_equal(h$`content-length`, "14")

  # Missing directory in path (404)
  r <- fetch(local_url("/foo/bar", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 404)
  expect_identical(rawToChar(r$content), "404 Not Found\n")
  expect_equal(h$`content-length`, "14")

  # MIME types for other files
  r <- fetch(local_url("/mtcars.csv", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(h$`content-type`, "text/csv")

  r <- fetch(local_url("/data.txt", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(h$`content-type`, "text/plain")
})


test_that("Missing file fallthrough", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        return(list(
          status = 404,
          headers = list("Test-Code-Path" = "R"),
          body = paste0("404 file not found: ", req$PATH_INFO)
        ))
      },
      staticPaths = list(
        # Testing out various leading and trailing slashes
        "/" = staticPath(
          test_path("apps/content"),
          indexhtml = FALSE,
          fallthrough = TRUE
        )
      )
    )
  )
  on.exit(s$stop())

  r <- fetch(local_url("/", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 404)
  expect_identical(h$`test-code-path`, "R")
  expect_identical(rawToChar(r$content), "404 file not found: /")
})


test_that("Longer paths override shorter ones", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      staticPaths = list(
        # Testing out various leading and trailing slashes
        "/" = test_path("apps/content"),
        "/a" = staticPath(
          test_path("apps/content"),
          indexhtml = FALSE
        ),
        "/a/b" = staticPath(
          test_path("apps/content"),
          indexhtml = NULL
        ),
        "/a/b/c" = staticPath(
          test_path("apps/content"),
          indexhtml = TRUE
        )
      )
    )
  )
  on.exit(s$stop())

  r <- fetch(local_url("/", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, index_file_content)

  r <- fetch(local_url("/a/", s$getPort()))
  expect_equal(r$status_code, 404)

  # When NULL, option values are not inherited from the parent dir, "/a";
  # they're inherited from the overall options for the app.
  r <- fetch(local_url("/a/b", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, index_file_content)

  r <- fetch(local_url("/a/b/c", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, index_file_content)
})


test_that("Options and option inheritance", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        return(list(
          status = 404,
          headers = list("Test-Code-Path" = "R"),
          body = paste0("404 file not found: ", req$PATH_INFO)
        ))
      },
      staticPaths = list(
        "/default" = staticPath(test_path("apps/content")),
        # This path overrides options
        "/override" = staticPath(
          test_path("apps/content"),
          indexhtml = FALSE,
          fallthrough = TRUE,
          html_charset = "ISO-8859-1",
          headers = list("Test-Code-Path" = "C++2")
        ),
        # This path unsets some options
        "/unset" = staticPath(
          test_path("apps/content"),
          html_charset = "",
          headers = list()
        )
      ),
      staticPathOptions = staticPathOptions(
        indexhtml = TRUE,
        fallthrough = FALSE,
        headers = list("Test-Code-Path" = "C++")
      )
    )
  )
  on.exit(s$stop())

  r <- fetch(local_url("/default", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 200)
  expect_identical(h$`content-type`, "text/html; charset=utf-8")
  expect_identical(h$`test-code-path`, "C++")
  expect_identical(r$content, index_file_content)

  r <- fetch(local_url("/override", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 404)
  expect_identical(h$`test-code-path`, "R")
  expect_identical(rawToChar(r$content), "404 file not found: /override")

  r <- fetch(local_url("/override/index.html", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 200)
  expect_identical(h$`test-code-path`, "C++2")
  expect_identical(h$`content-type`, "text/html; charset=ISO-8859-1")

  r <- fetch(local_url("/unset", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 200)
  expect_false("test-code-path" %in% names(h))
  expect_identical(h$`content-type`, "text/html")
  expect_identical(r$content, index_file_content)

  r <- fetch(local_url("/unset/index.html", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 200)
  expect_false("test-code-path" %in% names(h))
  expect_identical(h$`content-type`, "text/html")
  expect_identical(r$content, index_file_content)
})


test_that("Excluding subpaths", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        # Return a 403 for the R code path; the C++ code path will return 404
        # for missing files.
        return(list(
          status = 403,
          headers = list("Test-Code-Path" = "R"),
          body = paste0("403 forbidden: ", req$PATH_INFO)
        ))
      },
      staticPaths = list(
        "/" = staticPath(test_path("apps/content")),
        "/exclude" = excludeStaticPath(),
        "/subdi" = excludeStaticPath(),

        "/a" = staticPath(test_path("apps/content")),
        "/a/exclude" = excludeStaticPath(),
        "/a/mtcars.csv" = excludeStaticPath()
      )
    )
  )
  on.exit(s$stop())

  exclude_subdir_index_file_content <- raw_file_content(test_path("apps/content/exclude/subdir/index.html"))

  # Basic test
  r <- fetch(local_url("/", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, index_file_content)
  r <- fetch(local_url("/subdir", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, subdir_index_file_content)
  r <- fetch(local_url("/exclude", s$getPort()))
  expect_equal(r$status_code, 403)
  r <- fetch(local_url("/exclude/index.html", s$getPort()))
  expect_equal(r$status_code, 403)
  r <- fetch(local_url("/exclude/subdir", s$getPort()))
  expect_equal(r$status_code, 403)
  r <- fetch(local_url("/exclude/subdir/index.html", s$getPort()))
  expect_equal(r$status_code, 403)

  # Include directories underneath excluded dir.
  s$setStaticPath("exclude/include" = test_path("apps/content"))
  r <- fetch(local_url("/exclude/include", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, index_file_content)

  s$setStaticPath("exclude/subdir" = test_path("apps/content/exclude/subdir"))
  r <- fetch(local_url("/exclude/subdir", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, exclude_subdir_index_file_content)

  # A file that is not specifically excluded will use the C++ 404 path.
  r <- fetch(local_url("/nonexistent.txt", s$getPort()))
  expect_equal(r$status_code, 404)

  # Fallthrough. Behavior should be unchanged except for non-existent files that
  # are NOT in the excluded path.
  s$setStaticPathOption(fallthrough = TRUE)
  # Now, a file that is not specifically excluded will use the R 403 path
  r <- fetch(local_url("/nonexistent.txt", s$getPort()))
  expect_equal(r$status_code, 403)
  s$setStaticPathOption(fallthrough = FALSE)

  # Partial name matching ("subdi" was excluded) doesn't work.
  r <- fetch(local_url("/subdir", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, subdir_index_file_content)

  # Specific files
  r <- fetch(local_url("/a/", s$getPort()))
  expect_equal(r$status_code, 200)
  r <- fetch(local_url("/a/mtcars.csv", s$getPort()))
  expect_equal(r$status_code, 403)
  # A file that is not specifically excluded will use the C++ 404 path.
  r <- fetch(local_url("/file/nonexistent.txt", s$getPort()))
  expect_equal(r$status_code, 404)
})

test_that("Header validation", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        if (!identical(req$HTTP_TEST_VALIDATION, "aaa")) {
          return(list(
            status = 403,
            headers = list("Test-Code-Path" = "R"),
            body = "403 Forbidden\n"
          ))
        }
        return(list(
          status = 200,
          headers = list("Test-Code-Path" = "R"),
          body = "200 OK\n"
        ))
      },
      staticPaths = list(
        "/default" = staticPath(test_path("apps/content")),
        # This path overrides validation
        "/override" = staticPath(
          test_path("apps/content"),
          validation = c('"Test-Validation-1" == "bbb"')
        ),
        # This path unsets validation
        "/unset" = staticPath(
          test_path("apps/content"),
          validation = character()
        ),
        # Fall through to R
        "/fallthrough" = staticPath(
          test_path("apps/content"),
          fallthrough = TRUE
        )
      ),
      staticPathOptions = staticPathOptions(
        headers = list("Test-Code-Path" = "C++"),
        validation = c('"Test-Validation" == "aaa"')
      )
    )
  )
  on.exit(s$stop())

  r <- fetch(local_url("/default", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 403)
  # This header doesn't get set. Should it?
  expect_false("test-code-path" %in% names(h))
  expect_identical(rawToChar(r$content), "403 Forbidden\n")

  r <- fetch(local_url("/default", s$getPort()),
    handle_setheaders(new_handle(), "test-validation" = "aaa"))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 200)
  expect_identical(h$`test-code-path`, "C++")
  expect_identical(r$content, index_file_content)

  # Check case insensitive
  r <- fetch(local_url("/default", s$getPort()),
    handle_setheaders(new_handle(), "tesT-ValidatioN" = "aaa"))
  expect_equal(r$status_code, 200)

  r <- fetch(local_url("/unset", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 200)
  expect_identical(h$`test-code-path`, "C++")
  expect_identical(r$content, index_file_content)

  # When fallthrough=TRUE, the header validation is still checked before falling
  # through to the R code path.
  r <- fetch(local_url("/fallthrough/missingfile", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 403)
  # This header doesn't get set. Should it?
  expect_false("test-code-path" %in% names(h))
  expect_identical(rawToChar(r$content), "403 Forbidden\n")

  r <- fetch(local_url("/fallthrough/missingfile", s$getPort()),
    handle_setheaders(new_handle(), "test-validation" = "aaa"))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 200)
  expect_identical(h$`test-code-path`, "R")
  expect_identical(rawToChar(r$content), "200 OK\n")
})


test_that("Dynamically changing paths", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        list(
          status = 500,
          headers = list("Test-Code-Path" = "R"),
          body = "500 Internal Server Error\n"
        )
      },
      staticPaths = list(
        "/static" = test_path("apps/content")
      )
    )
  )
  on.exit(s$stop())

  r <- fetch(local_url("/static", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, index_file_content)

  # Replace with different static path and options
  s$setStaticPath(
    "/static" = staticPath(
      test_path("apps/content_1"),
      indexhtml = FALSE
    )
  )

  r <- fetch(local_url("/static", s$getPort()))
  expect_equal(r$status_code, 404)

  r <- fetch(local_url("/static/index.html", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, index_file_1_content)

  # Remove static path
  s$removeStaticPath("/static")

  expect_equal(length(s$getStaticPaths()), 0)

  r <- fetch(local_url("/static", s$getPort()))
  expect_equal(r$status_code, 500)
  h <- parse_headers_list(r$headers)
  expect_identical(h$`test-code-path`, "R")
  expect_identical(rawToChar(r$content), "500 Internal Server Error\n")

  # Add static path
  s$setStaticPath(
    "/static_new" = test_path("apps/content")
  )
  r <- fetch(local_url("/static_new", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(r$content, index_file_content)
})


test_that("Dynamically changing options", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        list(
          status = 500,
          headers = list("Test-Code-Path" = "R"),
          body = "500 Internal Server Error\n"
        )
      },
      staticPaths = list(
        "/static" = test_path("apps/content")
      )
    )
  )
  on.exit(s$stop())

  r <- fetch(local_url("/static", s$getPort()))
  expect_equal(r$status_code, 200)

  s$setStaticPathOption(indexhtml = FALSE)
  r <- fetch(local_url("/static", s$getPort()))
  expect_equal(r$status_code, 404)

  s$setStaticPathOption(fallthrough = TRUE)
  r <- fetch(local_url("/static", s$getPort()))
  expect_equal(r$status_code, 500)

  s$setStaticPathOption(
    indexhtml = TRUE,
    headers = list("Test-Headers" = "aaa"),
    validation = c('"Test-Validation" == "aaa"')
  )
  r <- fetch(local_url("/static", s$getPort()))
  expect_equal(r$status_code, 403)
  r <- fetch(local_url("/static", s$getPort()),
    handle_setheaders(new_handle(), "test-validation" = "aaa"))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 200)
  expect_identical(h$`test-headers`, "aaa")

  # Unset some options
  s$setStaticPathOption(
    headers = list(),
    validation = character()
  )
  r <- fetch(local_url("/static", s$getPort()))
  h <- parse_headers_list(r$headers)
  expect_equal(r$status_code, 200)
  expect_false("test-headers" %in% h)
})


test_that("Escaped characters in paths", {
  # Need to create files with weird names
  static_dir <- tempfile("httpuv_test")
  dir.create(static_dir)
  # Use writeBin() instead of cat() because in Windows, cat() will convert "\n"
  # to "\r\n".
  writeBin(charToRaw("This is file content.\n"), file.path(static_dir, "file with space.txt"))
  on.exit(unlink(static_dir, recursive = TRUE))


  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        list(
          status = 500,
          headers = list("Test-Code-Path" = "R"),
          body = "500 Internal Server Error\n"
        )
      },
      staticPaths = list(
        "/static" = static_dir
      )
    )
  )
  on.exit(s$stop(), add = TRUE)

  r <- fetch(local_url("/static/file%20with%20space.txt", s$getPort()))
  expect_equal(r$status_code, 200)
  expect_identical(rawToChar(r$content), "This is file content.\n")
})


test_that("Paths with ..", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        list(
          status = 404,
          headers = list("Test-Code-Path" = "R"),
          body = "404 Not Found\n"
        )
      },
      staticPaths = list(
        "/static" = test_path("apps/content")
      )
    )
  )
  on.exit(s$stop())

  # Need to use http_request_con() instead of fetch() to send custom requests
  # with "..".
  res <- http_request_con("GET /", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 404 Not Found")
  expect_true(any(grepl("^Test-Code-Path: R$", res, ignore.case = TRUE)))

  res <- http_request_con("GET /static", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 200 OK")

  # The presence of a ".." path segment results in a 400.
  res <- http_request_con("GET /static/..", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 400 Bad Request")

  res <- http_request_con("GET /static/../", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 400 Bad Request")

  res <- http_request_con("GET /static/../static", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 400 Bad Request")

  # ".." is valid as part of a path segment (but we'll get 404's since the files
  # don't actually exist).
  res <- http_request_con("GET /static/..foo", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 404 Not Found")
  expect_false(any(grepl("^Test-Code-Path: R$", res, ignore.case = TRUE)))

  res <- http_request_con("GET /static/foo..", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 404 Not Found")
  expect_false(any(grepl("^Test-Code-Path: R$", res, ignore.case = TRUE)))

  res <- http_request_con("GET /static/foo../", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 404 Not Found")
  expect_false(any(grepl("^Test-Code-Path: R$", res, ignore.case = TRUE)))


})

test_that("Paths with backslash", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        list(
          status = 400,
          headers = list("Test-Code-Path" = "R"),
          body = "400 Bad Request\n"
        )
      },
      staticPaths = list(
        "/static" = test_path("apps/content")
      )
    )
  )
  on.exit(s$stop())

  # Need to use http_request_con() instead of fetch() to send custom requests
  # with "..".
  # When a backslash is in path, should fall through to R code path.

  # Raw backslash
  res <- http_request_con("GET /static\\index.html", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 400 Bad Request")
  expect_true(any(grepl("^Test-Code-Path: R$", res, ignore.case = TRUE)))

  # Escaped backslash
  res <- http_request_con("GET /static%5cindex.html", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 400 Bad Request")
  expect_true(any(grepl("^Test-Code-Path: R$", res, ignore.case = TRUE)))

  # Raw backslash with ..
  res <- http_request_con("GET /static/..\\index.html", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 400 Bad Request")
  expect_true(any(grepl("^Test-Code-Path: R$", res, ignore.case = TRUE)))

  # Escaped backslash with ..
  res <- http_request_con("GET /static/..%5cindex.html", "127.0.0.1", s$getPort())
  expect_identical(res[1], "HTTP/1.1 400 Bad Request")
  expect_true(any(grepl("^Test-Code-Path: R$", res, ignore.case = TRUE)))
})

test_that("HEAD, POST, PUT requests", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      call = function(req) {
        list(
          status = 404,
          headers = list("Test-Code-Path" = "R"),
          body = "404 Not Found\n"
        )
      },
      staticPaths = list(
        "/static" = test_path("apps/content")
      )
    )
  )
  on.exit(s$stop())

  # The GET results, for comparison to HEAD.
  r_get <- fetch(local_url("/static", s$getPort()))
  h_get <- parse_headers_list(r_get$headers)

  # HEAD is OK.
  # Note the weird interface for a HEAD request:
  # https://github.com/jeroen/curl/issues/24
  r <- fetch(local_url("/static", s$getPort()), new_handle(nobody = TRUE))
  expect_equal(r$status_code, 200)
  expect_true(length(r$content) == 0)  # No message body for HEAD
  h <- parse_headers_list(r$headers)
  # Headers should match GET request, except for date.
  expect_identical(h[setdiff(names(h), "date")], h_get[setdiff(names(h_get), "date")])

  # POST and PUT are not OK
  r <- fetch(local_url("/static", s$getPort()),
    handle_setopt(new_handle(), customrequest = "POST"))
  expect_equal(r$status_code, 400)

  r <- fetch(local_url("/static", s$getPort()),
    handle_setopt(new_handle(), customrequest = "PUT"))
  expect_equal(r$status_code, 400)
})



test_that("Last-Modified and If-Modified-Since headers", {
  s <- startServer("127.0.0.1", randomPort(),
    list(
      staticPaths = list(
        "/" = staticPath(
          test_path("apps/content"),
          headers = list(
            "ETag" = "abc",
            "Cache-Control" = "max-age=12345",
            "Other" = "xyz"
          )
        )
      )
    )
  )
  on.exit(s$stop())

  # mtime of the target file, rounded down to nearest second.
  file_mtime <- as.POSIXct(trunc(file.info(test_path("apps/content/mtcars.csv"))$mtime))

  # First time retrieving: no Last-Modified header.
  r <- fetch(local_url("/mtcars.csv", s$getPort()))
  h <- parse_headers_list(r$headers)
  http_mtime <- r$modified
  expect_equal(file_mtime, http_mtime)


  # Use the Last-Modified value in the If-Modified-Since header.
  r1 <- fetch(local_url("/mtcars.csv", s$getPort()),
    handle_setheaders(new_handle(),
      "If-Modified-Since" = h$`last-modified`
    )
  )
  expect_identical(r1$status_code, 304L)
  expect_true(length(r1$content) == 0)
  h1 <- parse_headers_list(r1$headers)
  # A 304 response should contain only the following headers (and must contain
  # them if the corresponding 200 response would have them):
  # Cache-Control, Content-Location, Date, ETag, Expires, Vary
  # https://httpstatuses.com/304
  expect_identical(h[c("cache-control", "etag")], h1[c("cache-control", "etag")])
  # The Date header differs from the previous response because the request was
  # made at a different time. We just need to check that it's present.
  expect_true("date" %in% names(h1))


  # The mtime plus 1 second should result in a 304.
  r1 <- fetch(local_url("/mtcars.csv", s$getPort()),
    handle_setheaders(new_handle(),
      "If-Modified-Since" = http_date_string(file_mtime + 1)
    )
  )
  expect_identical(r1$status_code, 304L)


  # Last-Modified header minus 1 second should result in a regular 200 response.
  r1 <- fetch(local_url("/mtcars.csv", s$getPort()),
    handle_setheaders(new_handle(),
      "If-Modified-Since" = http_date_string(file_mtime - 1)
    )
  )
  expect_identical(r1$status_code, 200L)
  h1 <- parse_headers_list(r1$headers)
  expect_identical(h[setdiff(names(h), "date")], h1[setdiff(names(h1), "date")])


  # Malformed If-Modified-Since value should be ignored.
  #
  # First, a date far in the future should result in 304. Note that the 2038
  # date is used here because on 32-bit Windows, dates that are beyond
  # 2038-01-19 will overflow and wrap around, and this request will get a 200
  # instead of 304. Other platforms seem not to have this limitation.
  r1 <- fetch(local_url("/mtcars.csv", s$getPort()),
    handle_setheaders(new_handle(),
      "If-Modified-Since" = "Mon, 01 Jan 2038 12:00:00 GMT"
    )
  )
  expect_identical(r1$status_code, 304L)
  # Next, almost the same date, but slightly malformed, should result in 200.
  r1 <- fetch(local_url("/mtcars.csv", s$getPort()),
    handle_setheaders(new_handle(),
      "If-Modified-Since" = "Mon, 01 Jan 2038 12:100:00 GMT"
    )
  )
  expect_identical(r1$status_code, 200L)
})


test_that("Paths with non-ASCII characters", {
  # "apps/fü", in UTF-8 encoding.
  nonascii_path <- test_path("apps/f\U00FC")
  dir.create(nonascii_path)
  on.exit(unlink(nonascii_path, recursive = TRUE))

  index_file_path <- file.path(nonascii_path, "index.html")
  writeLines("Hello world!", index_file_path)
  file_content <- raw_file_content(index_file_path)

  s <- startServer("0.0.0.0", randomPort(),
    list(
      call = function(req) {
        list(
          status = 200L,
          headers = list('Content-Type' = 'text/html'),
          body = "R code path"
        )
      },
      staticPaths = list(
        "/f\U00FC" = nonascii_path,
        "/foo" = nonascii_path
      )
    )
  )
  on.exit(s$stop(), add = TRUE)

  # URL-encoded non-ASCII URL path, which maps to non-ASCII local path.
  r <- fetch(local_url("/f%C3%BC", s$getPort()))
  expect_identical(r$status_code, 200L)
  expect_identical(r$content, file_content)

  # ASCII URL path, which maps to non-ASCII local path.
  r <- fetch(local_url("/foo", s$getPort()))
  expect_identical(r$status_code, 200L)
  expect_identical(r$content, file_content)
})
