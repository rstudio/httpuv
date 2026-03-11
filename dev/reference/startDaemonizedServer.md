# Create an HTTP/WebSocket daemonized server (deprecated)

This function will be removed in a future release of httpuv. It is
simply a wrapper for
[`startServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md).
In previous versions of httpuv (1.3.5 and below), `startServer` ran
applications in the foreground and `startDaemonizedServer` ran
applications in the background, but now both of them run applications in
the background.

## Usage

``` r
startDaemonizedServer(host, port, app, quiet = FALSE)
```

## Arguments

- host:

  A string that is a valid IPv4 address that is owned by this server, or
  `"0.0.0.0"` to listen on all IP addresses.

- port:

  A number or integer that indicates the server port that should be
  listened on. Note that on most Unix-like systems including Linux and
  macOS, port numbers smaller than 1024 require root privileges.

- app:

  A collection of functions that define your application. See Details.

- quiet:

  If `TRUE`, suppress error messages from starting app.
