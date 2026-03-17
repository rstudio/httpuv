# Serve a directory

`runStaticServer()` provides a convenient interface to start a server to
host a single static directory, either in the foreground or the
background.

## Usage

``` r
runStaticServer(
  dir = getwd(),
  host = "127.0.0.1",
  port = NULL,
  ...,
  background = FALSE,
  browse = interactive()
)
```

## Arguments

- dir:

  The directory to serve. Defaults to the current working directory.

- host:

  A string that is a valid IPv4 address that is owned by this server, or
  `"0.0.0.0"` to listen on all IP addresses.

- port:

  A number or integer that indicates the server port that should be
  listened on. Note that on most Unix-like systems including Linux and
  macOS, port numbers smaller than 1024 require root privileges.

- ...:

  Arguments passed on to
  [`staticPath`](https://rstudio.github.io/httpuv/reference/staticPath.md)

  `path`

  :   The local path.

  `indexhtml`

  :   If an index.html file is present, should it be served up when the
      client requests the static path or any subdirectory?

  `fallthrough`

  :   With the default value, `FALSE`, if a request is made for a file
      that doesn't exist, then httpuv will immediately send a 404
      response from the background I/O thread, without needing to call
      back into the main R thread. This offers the best performance. If
      the value is `TRUE`, then instead of sending a 404 response,
      httpuv will call the application's `call` function, and allow it
      to handle the request.

  `html_charset`

  :   When HTML files are served, the value that will be provided for
      `charset` in the Content-Type header. For example, with the
      default value, `"utf-8"`, the header is
      `Content-Type: text/html; charset=utf-8`. If `""` is used, then no
      `charset` will be added in the Content-Type header.

  `headers`

  :   Additional headers and values that will be included in the
      response.

  `validation`

  :   An optional validation pattern. Presently, the only type of
      validation supported is an exact string match of a header. For
      example, if `validation` is `'"abc" = "xyz"'`, then HTTP requests
      must have a header named `abc` (case-insensitive) with the value
      `xyz` (case-sensitive). If a request does not have a matching
      header, than httpuv will give a 403 Forbidden response. If the
      `character(0)` (the default), then no validation check will be
      performed.

- background:

  Whether to run the server in the background. By default, the server
  runs in the foreground and blocks the R console. You can stop the
  server by interrupting it with `Ctrl + C`.

  When `background = TRUE`, the server will run in the background and
  will process requests when the R console is idle. To stop a background
  server, call
  [`stopAllServers()`](https://rstudio.github.io/httpuv/reference/stopAllServers.md)
  or call
  [`stopServer()`](https://rstudio.github.io/httpuv/reference/stopServer.md)
  on the server object returned (invisibly) by this function.

- browse:

  Whether to automatically open the served directory in a web browser.
  Defaults to `TRUE` when running interactively.

## Value

Starts a server on the specified host and port. By default the server
runs in the foreground and is accessible at `http://127.0.0.1:7446`.
When `background = TRUE`, the `server` object is returned invisibly.

## See also

[`runServer()`](https://rstudio.github.io/httpuv/reference/runServer.md)
provides a similar interface for running a dynamic app server. Both
`runStaticServer()` and
[`runServer()`](https://rstudio.github.io/httpuv/reference/runServer.md)
are built on top of
[`startServer()`](https://rstudio.github.io/httpuv/reference/startServer.md),
[`service()`](https://rstudio.github.io/httpuv/reference/service.md) and
[`stopServer()`](https://rstudio.github.io/httpuv/reference/stopServer.md).
Learn more about httpuv servers in
[`startServer()`](https://rstudio.github.io/httpuv/reference/startServer.md).

## Examples

``` r
if (FALSE) { # interactive()
website_dir <- system.file("example-static-site", package = "httpuv")
runStaticServer(dir = website_dir)
}
```
