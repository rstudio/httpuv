# PipeServer class

This class represents a server running one application that listens on a
named pipe.

## See also

[`Server()`](https://rstudio.github.io/httpuv/dev/reference/Server.md)
and
[`WebServer()`](https://rstudio.github.io/httpuv/dev/reference/WebServer.md).

## Super class

[`Server`](https://rstudio.github.io/httpuv/dev/reference/Server.md) -\>
`PipeServer`

## Methods

### Public methods

- [`PipeServer$new()`](#method-PipeServer-initialize)

- [`PipeServer$getName()`](#method-PipeServer-getName)

- [`PipeServer$getMask()`](#method-PipeServer-getMask)

Inherited methods

- [`Server$getStaticPathOptions()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-getStaticPathOptions)
- [`Server$getStaticPaths()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-getStaticPaths)
- [`Server$isRunning()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-isRunning)
- [`Server$removeStaticPath()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-removeStaticPath)
- [`Server$setStaticPath()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-setStaticPath)
- [`Server$setStaticPathOption()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-setStaticPathOption)
- [`Server$stop()`](https://rstudio.github.io/httpuv/dev/reference/Server.html#method-stop)

------------------------------------------------------------------------

### `PipeServer$new()`

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

------------------------------------------------------------------------

### `PipeServer$getName()`

Get the name of the named pipe

#### Usage

    PipeServer$getName()

#### Returns

The name of the named pipe that the server is bound to.

------------------------------------------------------------------------

### `PipeServer$getMask()`

Get the mask for the named pipe

#### Usage

    PipeServer$getMask()

#### Returns

The mask for the named pipe that the server is bound to.

## Examples

``` r

## ------------------------------------------------
## Method `PipeServer$new()`
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
