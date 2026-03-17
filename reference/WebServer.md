# WebServer class

This class represents a web server running one application. Multiple
servers can be running at the same time.

## See also

[`Server()`](https://rstudio.github.io/httpuv/reference/Server.md) and
[`PipeServer()`](https://rstudio.github.io/httpuv/reference/PipeServer.md).

## Super class

[`httpuv::Server`](https://rstudio.github.io/httpuv/reference/Server.md)
-\> `WebServer`

## Methods

### Public methods

- [`WebServer$new()`](#method-WebServer-new)

- [`WebServer$getHost()`](#method-WebServer-getHost)

- [`WebServer$getPort()`](#method-WebServer-getPort)

Inherited methods

- [`httpuv::Server$getStaticPathOptions()`](https://rstudio.github.io/httpuv/reference/Server.html#method-getStaticPathOptions)
- [`httpuv::Server$getStaticPaths()`](https://rstudio.github.io/httpuv/reference/Server.html#method-getStaticPaths)
- [`httpuv::Server$isRunning()`](https://rstudio.github.io/httpuv/reference/Server.html#method-isRunning)
- [`httpuv::Server$removeStaticPath()`](https://rstudio.github.io/httpuv/reference/Server.html#method-removeStaticPath)
- [`httpuv::Server$setStaticPath()`](https://rstudio.github.io/httpuv/reference/Server.html#method-setStaticPath)
- [`httpuv::Server$setStaticPathOption()`](https://rstudio.github.io/httpuv/reference/Server.html#method-setStaticPathOption)
- [`httpuv::Server$stop()`](https://rstudio.github.io/httpuv/reference/Server.html#method-stop)

------------------------------------------------------------------------

### Method `new()`

Initialize a new WebServer object

Create a new `WebServer` object. `app` is an httpuv application object
as described in
[`startServer()`](https://rstudio.github.io/httpuv/reference/startServer.md).

#### Usage

    WebServer$new(host, port, app, quiet = FALSE)

#### Arguments

- `host`:

  The host name or IP address to bind the server to.

- `port`:

  The port number to bind the server to.

- `app`:

  An httpuv application object as described in
  [`startServer()`](https://rstudio.github.io/httpuv/reference/startServer.md).

- `quiet`:

  If TRUE, suppresses output from the server.

#### Returns

A new `WebServer` object.

#### Examples

    \dontrun{
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
    }

------------------------------------------------------------------------

### Method `getHost()`

Get the host name or IP address of the server

#### Usage

    WebServer$getHost()

#### Returns

The host name or IP address that the server is bound to.

------------------------------------------------------------------------

### Method `getPort()`

Get the port number of the server

#### Usage

    WebServer$getPort()

#### Returns

The port number that the server is bound to.

## Examples

``` r

## ------------------------------------------------
## Method `WebServer$new`
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
