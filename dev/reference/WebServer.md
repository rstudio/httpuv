# WebServer class

This class represents a web server running one application. Multiple
servers can be running at the same time.

## See also

[`Server()`](https://rstudio.github.io/httpuv/dev/reference/Server.md)
and
[`PipeServer()`](https://rstudio.github.io/httpuv/dev/reference/PipeServer.md).

## Super class

[`Server`](https://rstudio.github.io/httpuv/dev/reference/Server.md) -\>
`WebServer`

## Methods

### Public methods

- [`WebServer$new()`](#method-WebServer-initialize)

- [`WebServer$getHost()`](#method-WebServer-getHost)

- [`WebServer$getPort()`](#method-WebServer-getPort)

Inherited methods

- [`Server$getStaticPathOptions()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-getStaticPathOptions)
- [`Server$getStaticPaths()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-getStaticPaths)
- [`Server$isRunning()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-isRunning)
- [`Server$removeStaticPath()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-removeStaticPath)
- [`Server$setStaticPath()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-setStaticPath)
- [`Server$setStaticPathOption()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-setStaticPathOption)
- [`Server$stop()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-stop)

------------------------------------------------------------------------

### `WebServer$new()`

Initialize a new WebServer object

Create a new `WebServer` object. `app` is an httpuv application object
as described in
[`startServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md).

#### Usage

    WebServer$new(host, port, app, quiet = FALSE)

#### Arguments

- `host`:

  The host name or IP address to bind the server to.

- `port`:

  The port number to bind the server to.

- `app`:

  An httpuv application object as described in
  [`startServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md).

- `quiet`:

  If TRUE, suppresses output from the server.

#### Returns

A new `WebServer` object.

#### Examples

    # Create a simple app
    app <- function(req) {
      list(
        status = 200L,
        headers = list('Content-Type' = 'text/plain'),
        body = "Hello, world!"
      )
    }
    # Create a server
    server <- WebServer$new("127.0.0.1", 8080, app)

------------------------------------------------------------------------

### `WebServer$getHost()`

Get the host name or IP address of the server

#### Usage

    WebServer$getHost()

#### Returns

The host name or IP address that the server is bound to.

------------------------------------------------------------------------

### `WebServer$getPort()`

Get the port number of the server

#### Usage

    WebServer$getPort()

#### Returns

The port number that the server is bound to.

## Examples

``` r

## ------------------------------------------------
## Method `WebServer$new()`
## ------------------------------------------------

if (FALSE) { # \dontrun{
# Create a simple app
app <- function(req) {
  list(
    status = 200L,
    headers = list('Content-Type' = 'text/plain'),
    body = "Hello, world!"
  )
}
# Create a server
server <- WebServer$new("127.0.0.1", 8080, app)
} # }
```
