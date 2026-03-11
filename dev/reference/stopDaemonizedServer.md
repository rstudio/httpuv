# Stop a running daemonized server in Unix environments (deprecated)

This function will be removed in a future release of httpuv. Instead,
use
[`stopServer()`](https://rstudio.github.io/httpuv/dev/reference/stopServer.md).

## Usage

``` r
stopDaemonizedServer(server)
```

## Arguments

- server:

  A server object that was previously returned from
  [`startServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md)
  or
  [`startPipeServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md).
