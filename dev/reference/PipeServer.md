# PipeServer class

This class represents a server running one application that listens on a
named pipe.

## See also

[`Server()`](https://rstudio.github.io/httpuv/dev/reference/Server.md)
and
[`WebServer()`](https://rstudio.github.io/httpuv/dev/reference/WebServer.md).

## Super class

[`httpuv::Server`](https://rstudio.github.io/httpuv/dev/reference/Server.md)
-\> `PipeServer`

## Methods

### Public methods

- [`PipeServer$new()`](#method-PipeServer-new)

- [`PipeServer$getName()`](#method-PipeServer-getName)

- [`PipeServer$getMask()`](#method-PipeServer-getMask)

Inherited methods

- [`httpuv::Server$getStaticPathOptions()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-getStaticPathOptions)
- [`httpuv::Server$getStaticPaths()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-getStaticPaths)
- [`httpuv::Server$isRunning()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-isRunning)
- [`httpuv::Server$removeStaticPath()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-removeStaticPath)
- [`httpuv::Server$setStaticPath()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-setStaticPath)
- [`httpuv::Server$setStaticPathOption()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-setStaticPathOption)
- [`httpuv::Server$stop()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-stop)

------------------------------------------------------------------------

### Method `new()`

Initialize a new PipeServer object

Create a new `PipeServer` object. `app` is an httpuv application object
as described in
[`startServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md).

#### Usage

    PipeServer$new(name, mask, app, quiet = FALSE)

#### Arguments

- `name`:

  The name of the named pipe to bind the server to.

- `mask`:

  The mask for the named pipe. If NULL, it defaults to -1.

- `app`:

  An httpuv application object as described in
  [`startServer()`](https://rstudio.github.io/httpuv/dev/reference/startServer.md).

- `quiet`:

  If TRUE, suppresses output from the server.

#### Returns

A new `PipeServer` object.

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
    server <- PipeServer$new("my_pipe", -1, app)
    }

------------------------------------------------------------------------

### Method `getName()`

Get the name of the named pipe

#### Usage

    PipeServer$getName()

#### Returns

The name of the named pipe that the server is bound to.

------------------------------------------------------------------------

### Method `getMask()`

Get the mask for the named pipe

#### Usage

    PipeServer$getMask()

#### Returns

The mask for the named pipe that the server is bound to.

## Examples

``` r

## ------------------------------------------------
## Method `PipeServer$new`
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
server <- PipeServer$new("my_pipe", -1, app)
} # }
```
