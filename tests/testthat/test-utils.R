context("utils")

test_that("encodeURI and encodeURIComponent", {
  # "abc \ue5 \u4e2d" is identical to "abc å 中" when the system's encoding is
  # UTF-8. However, the former is always encoded as UTF-8, while the latter will
  # be encoded using the system's native encoding.
  utf8_str <- "abc \ue5 \u4e2d"
  expect_true(Encoding(utf8_str) == "UTF-8")

  expect_identical(encodeURI(utf8_str), "abc%20%C3%A5%20%E4%B8%AD")
  expect_identical(encodeURIComponent(utf8_str), "abc%20%C3%A5%20%E4%B8%AD")
  expect_identical(decodeURI("abc%20%C3%A5%20%E4%B8%AD"), utf8_str)
  expect_identical(decodeURIComponent("abc%20%C3%A5%20%E4%B8%AD"), utf8_str)
  expect_true(Encoding(decodeURI("abc%20%C3%A5%20%E4%B8%AD")) == "UTF-8")
  expect_true(Encoding(decodeURIComponent("abc%20%C3%A5%20%E4%B8%AD")) == "UTF-8")

  # Behavior with reserved characters differs between encodeURI and
  # encodeURIComponent.
  expect_identical(encodeURI(",/?:@"), ",/?:@")
  expect_identical(encodeURIComponent(",/?:@"), "%2C%2F%3F%3A%40")
  expect_identical(decodeURI("%2C%2F%3F%3A%40"), "%2C%2F%3F%3A%40")
  expect_identical(decodeURIComponent("%2C%2F%3F%3A%40"), ",/?:@")

  # Decoding characters that aren't encoded should have no effect.
  expect_identical(decodeURI(utf8_str), utf8_str)
  expect_identical(decodeURIComponent(utf8_str), utf8_str)
  expect_true(Encoding(decodeURI(utf8_str)) == "UTF-8")
  expect_true(Encoding(decodeURIComponent(utf8_str)) == "UTF-8")
  expect_identical(decodeURI(",/?:@"), ",/?:@")
  expect_identical(decodeURIComponent(",/?:@"), ",/?:@")
})


test_that("ipFamily works correctly", {
  expect_identical(ipFamily("127.0.0.1"), 4L)
  expect_identical(ipFamily("0.0.0.0"), 4L)
  expect_identical(ipFamily("192.168.0.1"), 4L)

  expect_identical(ipFamily("::1"), 6L)
  expect_identical(ipFamily("::"), 6L)
  expect_identical(ipFamily("fe80::91:5800:400a:075c"), 6L)
  expect_identical(ipFamily("fe80::1"), 6L)

  # IPv6 with zone ID
  expect_identical(ipFamily("::1%lo0"), 6L)
  expect_identical(ipFamily("::%1"), 6L)
  expect_identical(ipFamily("fe80::91:5800:400a:075c%en0"), 6L)
  expect_identical(ipFamily("fe80::1%abcd"), 6L)


  expect_identical(ipFamily("fe80::91:5800:400a:%075c"), -1L)
  expect_identical(ipFamily(":::1"), -1L)
  expect_identical(ipFamily(":1"), -1L)
  expect_identical(ipFamily("127.0.0.1%1"), -1L)
  expect_identical(ipFamily("10.0.0.500"), -1L)
  expect_identical(ipFamily("0.0.200"), -1L)
  expect_identical(ipFamily("123"), -1L)
  expect_identical(ipFamily("localhost"), -1L)
})
