# HTTP and WebSocket server

HTTP and WebSocket server

## Details

Allows R code to listen for and interact with HTTP and WebSocket
clients, so you can serve web traffic directly out of your R process.
Implementation is based on [libuv](https://github.com/joyent/libuv) and
[http-parser](https://github.com/nodejs/http-parser).

This is a low-level library that provides little more than network I/O
and implementations of the HTTP and WebSocket protocols. For an easy way
to create web applications, try [Shiny](https://shiny.posit.co) instead.

## See also

[startServer](https://rstudio.github.io/httpuv/dev/reference/startServer.md)

## Author

Joe Cheng <joe@rstudio.com>

## Examples

``` r
if (FALSE) { # \dontrun{
demo("echo", package="httpuv")
} # }
```
