context("utils")

test_that("encodeURI and encodeURIComponent", {
  # "abc \ue5 \u4e2d" is identical to "abc å 中" when the system's encoding is
  # UTF-8. However, the former is always encoded as UTF-8, while the latter will
  # be encoded using the system's native encoding.
  utf8_str             <- "abc \ue5 \u4e2d"
  utf8_str_encoded     <- "abc%20%C3%A5%20%E4%B8%AD"
  reserved_str         <- ",/?:@"
  reserved_str_encoded <- "%2C%2F%3F%3A%40"

  expect_true(Encoding(utf8_str) == "UTF-8")

  expect_identical(encodeURI(utf8_str),                  utf8_str_encoded)
  expect_identical(encodeURIComponent(utf8_str),         utf8_str_encoded)
  expect_identical(decodeURI(utf8_str_encoded),          utf8_str)
  expect_identical(decodeURIComponent(utf8_str_encoded), utf8_str)
  expect_true(Encoding(decodeURI(utf8_str_encoded)) == "UTF-8")
  expect_true(Encoding(decodeURIComponent(utf8_str_encoded)) == "UTF-8")

  # Behavior with reserved characters differs between encodeURI and
  # encodeURIComponent.
  expect_identical(encodeURI(reserved_str),                  reserved_str)
  expect_identical(encodeURIComponent(reserved_str),         reserved_str_encoded)
  expect_identical(decodeURI(reserved_str_encoded),          reserved_str_encoded)
  expect_identical(decodeURIComponent(reserved_str_encoded), reserved_str)

  # Decoding characters that aren't encoded should have no effect.
  expect_identical(decodeURI(utf8_str),          utf8_str)
  expect_identical(decodeURIComponent(utf8_str), utf8_str)
  expect_true(Encoding(decodeURI(utf8_str)) == "UTF-8")
  expect_true(Encoding(decodeURIComponent(utf8_str)) == "UTF-8")
  expect_identical(decodeURI(reserved_str),          reserved_str)
  expect_identical(decodeURIComponent(reserved_str), reserved_str)

  # Vector input
  expect_identical(
    encodeURI(c(reserved_str, utf8_str)),
    c(reserved_str, utf8_str_encoded)
  )
  expect_identical(
    encodeURIComponent(c(reserved_str, utf8_str)),
    c(reserved_str_encoded, utf8_str_encoded)
  )
  expect_identical(
    decodeURI(c(reserved_str_encoded, utf8_str_encoded)),
    c(reserved_str_encoded, utf8_str)
  )
  expect_identical(
    decodeURIComponent(c(reserved_str_encoded, utf8_str_encoded)),
    c(reserved_str, utf8_str)
  )

  # NA handling
  expect_identical(encodeURI(NA_character_), NA_character_)
  expect_identical(encodeURIComponent(NA_character_), NA_character_)
  expect_identical(decodeURI(NA_character_), NA_character_)
  expect_identical(decodeURIComponent(NA_character_), NA_character_)

  # Strings that are not UTF-8 encoded should be automatically converted to
  # UTF-8 before URL-encoding.
  #
  # "å", in UTF-8. The previous string, with Chinese characters, can't be
  # converted to latin1.
  utf8_str <- "\ue5"
  latin1_str <- iconv(utf8_str, "UTF-8", "latin1")

  expect_identical(encodeURI(utf8_str), "%C3%A5")
  expect_identical(encodeURI(latin1_str), "%C3%A5")
  expect_identical(encodeURIComponent(utf8_str), "%C3%A5")
  expect_identical(encodeURIComponent(latin1_str), "%C3%A5")
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
