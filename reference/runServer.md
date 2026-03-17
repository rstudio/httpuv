# Run a server

This is a convenience function that provides a simple way to call
[`startServer()`](https://rstudio.github.io/httpuv/reference/startServer.md),
[`service()`](https://rstudio.github.io/httpuv/reference/service.md),
and
[`stopServer()`](https://rstudio.github.io/httpuv/reference/stopServer.md)
in the correct sequence. It does not return unless interrupted or an
error occurs.

## Usage

``` r
runServer(host, port, app, interruptIntervalMs = NULL)
```

## Arguments

- host:

  A string that is a valid IPv4 or IPv6 address that is owned by this
  server, which the application will listen on. `"0.0.0.0"` represents
  all IPv4 addresses and `"::/0"` represents all IPv6 addresses.

- port:

  A number or integer that indicates the server port that should be
  listened on. Note that on most Unix-like systems including Linux and
  macOS, port numbers smaller than 1024 require root privileges.

- app:

  A collection of functions that define your application. See
  [`startServer()`](https://rstudio.github.io/httpuv/reference/startServer.md).

- interruptIntervalMs:

  Deprecated (last used in httpuv 1.3.5).

## Details

If you have multiple hosts and/or ports to listen on, call the
individual functions instead of `runServer`.

## See also

[`startServer()`](https://rstudio.github.io/httpuv/reference/startServer.md),
[`service()`](https://rstudio.github.io/httpuv/reference/service.md),
[`stopServer()`](https://rstudio.github.io/httpuv/reference/stopServer.md)

## Examples

``` r
if (FALSE) { # \dontrun{
# A very basic application
runServer("0.0.0.0", 5000,
  list(
    call = function(req) {
      list(
        status = 200L,
        headers = list(
          'Content-Type' = 'text/html'
        ),
        body = "Hello world!"
      )
    }
  )
)
} # }
```
