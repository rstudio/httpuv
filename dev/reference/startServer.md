# Create an HTTP/WebSocket server

Creates an HTTP/WebSocket server on the specified host and port.

## Usage

``` r
startServer(host, port, app, quiet = FALSE)

startPipeServer(name, mask, app, quiet = FALSE)
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

- name:

  A string that indicates the path for the domain socket (on Unix-like
  systems) or the name of the named pipe (on Windows).

- mask:

  If non-`NULL` and non-negative, this numeric value is used to
  temporarily modify the process's umask while the domain socket is
  being created. To ensure that only root can access the domain socket,
  use `strtoi("777", 8)`; or to allow owner and group read/write access,
  use `strtoi("117", 8)`. If the value is `NULL` then the process's
  umask is left unchanged. (This parameter has no effect on Windows.)

## Value

A handle for this server that can be passed to
[`stopServer()`](https://rstudio.github.io/httpuv/dev/reference/stopServer.md)
to shut the server down.

A
[`WebServer()`](https://rstudio.github.io/httpuv/dev/reference/WebServer.md)
or
[`PipeServer()`](https://rstudio.github.io/httpuv/dev/reference/PipeServer.md)
object.

## Details

`startServer` binds the specified port and listens for connections on an
thread running in the background. This background thread handles the
I/O, and when it receives a HTTP request, it will schedule a call to the
user-defined R functions in `app` to handle the request. This scheduling
is done with
[`later::later()`](https://later.r-lib.org/reference/later.html). When
the R call stack is empty – in other words, when an interactive R
session is sitting idle at the command prompt – R will automatically run
the scheduled calls. However, if the call stack is not empty – if R is
evaluating other R code – then the callbacks will not execute until
either the call stack is empty, or the
[`later::run_now()`](https://later.r-lib.org/reference/run_now.html)
function is called. This function tells R to execute any callbacks that
have been scheduled by
[`later::later()`](https://later.r-lib.org/reference/later.html). The
[`service()`](https://rstudio.github.io/httpuv/dev/reference/service.md)
function is essentially a wrapper for
[`later::run_now()`](https://later.r-lib.org/reference/run_now.html).

In older versions of httpuv (1.3.5 and below), it did not use a
background thread for I/O, and when this function was called, it did not
accept connections immediately. It was necessary to call
[`service()`](https://rstudio.github.io/httpuv/dev/reference/service.md)
repeatedly in order to actually accept and handle connections.

If the port cannot be bound (most likely due to permissions or because
it is already bound), an error is raised.

The application can also specify paths on the filesystem which will be
served from the background thread, without invoking `$call()` or
`$onHeaders()`. Files served this way will be only use a C++ code, which
is faster than going through R, and will not be blocked when R code is
executing. This can greatly improve performance when serving static
assets.

The `app` parameter is where your application logic will be provided to
the server. This can be a list, environment, or reference class that
contains the following methods and fields:

- `call(req)`:

  Process the given HTTP request, and return an HTTP response (see
  Response Values). This method should be implemented in accordance with
  the
  [Rook](https://github.com/jeffreyhorner/Rook/blob/a5e45f751/README.md)
  specification. Note that httpuv augments `req` with an additional
  item, `req$HEADERS`, which is a named character vector of request
  headers.

- `onHeaders(req)`:

  Optional. Similar to `call`, but occurs when headers are received.
  Return `NULL` to continue normal processing of the request, or a Rook
  response to send that response, stop processing the request, and ask
  the client to close the connection. (This can be used to implement
  upload size limits, for example.)

- `onWSOpen(ws)`:

  Called back when a WebSocket connection is established. The given
  object can be used to be notified when a message is received from the
  client, to send messages to the client, etc. See
  [`WebSocket()`](https://rstudio.github.io/httpuv/dev/reference/WebSocket.md).

- `staticPaths`:

  A named list of paths that will be served without invoking
  [`call()`](https://rdrr.io/r/base/call.html) or `onHeaders`. The name
  of each one is the URL path, and the value is either a string
  referring to a local path, or an object created by the
  [`staticPath()`](https://rstudio.github.io/httpuv/dev/reference/staticPath.md)
  function.

- `staticPathOptions`:

  A set of default options to use when serving static paths. If not set
  or `NULL`, then it will use the result from calling
  [`staticPathOptions()`](https://rstudio.github.io/httpuv/dev/reference/staticPathOptions.md)
  with no arguments.

The `startPipeServer` variant can be used instead of `startServer` to
listen on a Unix domain socket or named pipe rather than a TCP socket
(this is not common).

## Response Values

The `call` function is expected to return a list containing the
following, which are converted to an HTTP response and sent to the
client:

- `status`:

  A numeric HTTP status code, e.g. `200` or `404L`.

- `headers`:

  A named list of HTTP headers and their values, as strings. This can
  also be missing, an empty list, or `NULL`, in which case no headers
  (other than the `Date` and `Content-Length` headers, as required) will
  be added.

- `body`:

  A string (or `raw` vector) to be sent as the body of the HTTP
  response. This can also be omitted or set to `NULL` to avoid sending
  any body, which is useful for HTTP `1xx`, `204`, and `304` responses,
  as well as responses to `HEAD` requests.

## See also

[`stopServer()`](https://rstudio.github.io/httpuv/dev/reference/stopServer.md),
[`runServer()`](https://rstudio.github.io/httpuv/dev/reference/runServer.md),
[`listServers()`](https://rstudio.github.io/httpuv/dev/reference/listServers.md),
[`stopAllServers()`](https://rstudio.github.io/httpuv/dev/reference/stopAllServers.md).

## Examples

``` r
if (FALSE) { # \dontrun{
# A very basic application
s <- startServer("0.0.0.0", 5000,
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

s$stop()


# An application that serves static assets at the URL paths /assets and /lib
s <- startServer("0.0.0.0", 5000,
  list(
    call = function(req) {
      list(
        status = 200L,
        headers = list(
          'Content-Type' = 'text/html'
        ),
        body = "Hello world!"
      )
    },
    staticPaths = list(
      "/assets" = "content/assets/",
      "/lib" = staticPath(
        "content/lib",
        indexhtml = FALSE
      ),
      # This subdirectory of /lib should always be handled by the R code path
      "/lib/dynamic" = excludeStaticPath()
    ),
    staticPathOptions = staticPathOptions(
      indexhtml = TRUE
    )
  )
)

s$stop()
} # }
```
