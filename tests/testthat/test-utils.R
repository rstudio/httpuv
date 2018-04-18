context("utils")

test_that("encodeURI and encodeURIComponent", {
  skip("Temporarily disabled") # Because of https://github.com/rstudio/httpuv/issues/122

  expect_identical(encodeURI("abc å 中"), "abc%20%C3%A5%20%E4%B8%AD")
  expect_identical(encodeURIComponent("abc å 中"), "abc%20%C3%A5%20%E4%B8%AD")
  expect_identical(decodeURI("abc%20%C3%A5%20%E4%B8%AD"), "abc å 中")
  expect_identical(decodeURIComponent("abc%20%C3%A5%20%E4%B8%AD"), "abc å 中")

  # Behavior with reserved characters differs between encodeURI and
  # encodeURIComponent.
  expect_identical(encodeURI(",/?:@"), ",/?:@")
  expect_identical(encodeURIComponent(",/?:@"), "%2C%2F%3F%3A%40")
  expect_identical(decodeURI("%2C%2F%3F%3A%40"), "%2C%2F%3F%3A%40")
  expect_identical(decodeURIComponent("%2C%2F%3F%3A%40"), ",/?:@")

  # Decoding characters that aren't encoded should have no effect.
  expect_identical(decodeURI("abc å 中"), "abc å 中")
  expect_identical(decodeURIComponent("abc å 中"), "abc å 中")
  expect_identical(decodeURI(",/?:@"), ",/?:@")
  expect_identical(decodeURIComponent(",/?:@"), ",/?:@")
})
