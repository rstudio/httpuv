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
#' @useDynLib httpuv
"_PACKAGE"

## usethis namespace: start
#' @importFrom Rcpp evalCpp
#' @importFrom promises promise then finally is.promise %...>% %...!%
#' @importFrom later run_now
#' @importFrom R6 R6Class
## usethis namespace: end
NULL


otel_tracer_name <- "io.github.rstudio/httpuv"
