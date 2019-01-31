#' @rdname encodeURI
#' @export
decodeURI <- function(value) {
  res <- decodeURI_(value)
  Encoding(res) <- "UTF-8"
  res
}

#' @rdname encodeURI
#' @export
decodeURIComponent <- function(value) {
  res <- decodeURIComponent_(value)
  Encoding(res) <- "UTF-8"
  res
}
