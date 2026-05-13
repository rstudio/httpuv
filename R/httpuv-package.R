#' HTTP and WebSocket server
#'
#' Allows R code to listen for and interact with HTTP and WebSocket clients, so
#' you can serve web traffic directly out of your R process. Implementation is
#' based on [libuv](https://github.com/joyent/libuv) and
#' [http-parser](https://github.com/nodejs/http-parser).
#'
#' This is a low-level library that provides little more than network I/O and
#' implementations of the HTTP and WebSocket protocols. For an easy way to
#' create web applications, try [Shiny](https://shiny.posit.co) instead.
#'
#' @examples
#' \dontrun{
#' demo("echo", package="httpuv")
#' }
#'
#' @seealso [startServer]
#'
#' @name httpuv-package
#' @aliases httpuv
#' @docType package
#' @title HTTP and WebSocket server
#' @author Joe Cheng \email{joe@@rstudio.com}
#' @keywords package
#' @useDynLib httpuv, .registration = TRUE
"_PACKAGE"

## usethis namespace: start
#' @importFrom promises promise then finally is.promise %...>% %...!%
#' @importFrom later run_now
#' @importFrom R6 R6Class
## usethis namespace: end
NULL

# The following functions take C++ functions and provide an exported wrapper.
# The approach is
# R: myfun(); C++: myfun_().
# The C++ functions are registered with cpp11, and the R functions are exported.
# 'cpp4' (not in use) "saves" this step by allowing this on C++ side:
# /* roxygen
# @title My Function
# @param x something.
# @export
# */
# int myfun(int x) {
#   return x + 1;
# }

#' @title Apply the value of .Random.seed to R's internal RNG state
#' @description This function is needed in unusual cases where a C++ function calls
#'   an R function which sets the value of \code{.Random.seed}. This function
#'   should be called at the end of the R function to ensure that the new value
#'   \code{.Random.seed} is preserved.
#' @keywords internal
#' @export
getRNGState <- function() {
  getRNGState_()
}

#' @title URI encoding/decoding
#' @description Encodes/decodes strings using URI encoding/decoding in the same way that web
#' browsers do. The precise behaviors of these functions can be found at developer.mozilla.org:
#' \href{https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURI}{encodeURI},
#' \href{https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURIComponent}{encodeURIComponent},
#' \href{https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/decodeURI}{decodeURI},
#' \href{https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/decodeURIComponent}{decodeURIComponent}
#' 
#' Intended as a faster replacement for [utils::URLencode()] and [utils::URLdecode()].
#' encodeURI differs from encodeURIComponent in that the former will not encode
#' reserved characters: \code{;,/?:@@&=+$}
#' 
#' decodeURI differs from decodeURIComponent in that it will refuse to decode
#' encoded sequences that decode to a reserved character. (If in doubt, use
#' decodeURIComponent.)
#' 
#' For \code{encodeURI} and \code{encodeURIComponent}, input strings will be
#' converted to UTF-8 before URL-encoding.
#' @param value Character vector to be encoded or decoded.
#'
#' @return Encoded or decoded character vector of the same length as the
#'   input value. \code{decodeURI} and \code{decodeURIComponent} will return
#'   strings that are UTF-8 encoded.
#'
#' @export
encodeURI <- function(value) {
  encodeURI_(value)
}

#' @rdname encodeURI
#' @export
encodeURIComponent <- function(value) {
  encodeURIComponent_(value)
}

#' @rdname encodeURI
#' @export
decodeURI <- function(value) {
  decodeURI_(value)
}

#' @rdname encodeURI
#' @export
decodeURIComponent <- function(value) {
  decodeURIComponent_(value)
}

#' @title Check whether an address is IPv4 or IPv6
#' @description Given an IP address, this checks whether it is an IPv4 or IPv6 address.
#' @param ip A single string representing an IP address.
#' @return For IPv4 addresses, \code{4}; for IPv6 addresses, \code{6}. If the address is
#'   neither, \code{-1}.
#' @examples
#' ipFamily("127.0.0.1")   # 4
#' ipFamily("500.0.0.500") # -1
#' ipFamily("500.0.0.500") # -1
#'
#' ipFamily("::")          # 6
#' ipFamily("::1")         # 6
#' ipFamily("fe80::1ff:fe23:4567:890a") # 6
#' @export
ipFamily <- function(ip) {
  ipFamily_(ip)
}
