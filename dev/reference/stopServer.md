# Stop a server

Given a server object that was returned from a previous invocation of
[`startServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md)
or
[`startPipeServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md),
this closes all open connections for that server and unbinds the port.

## Usage

``` r
stopServer(server)
```

## Arguments

- server:

  A server object that was previously returned from
  [`startServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md)
  or
  [`startPipeServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md).

## See also

[`stopAllServers()`](https://rstudio.github.io/httpuv/dev/reference/stopAllServers.md)
to stop all servers.
