# Server class

The `Server` class is the parent class for
[`WebServer()`](https://rstudio.github.io/httpuv/dev/reference/WebServer.md)
and
[`PipeServer()`](https://rstudio.github.io/httpuv/dev/reference/PipeServer.md).
This class defines an interface and is not meant to be instantiated.

## See also

[`WebServer()`](https://rstudio.github.io/httpuv/dev/reference/WebServer.md)
and
[`PipeServer()`](https://rstudio.github.io/httpuv/dev/reference/PipeServer.md).

## Methods

### Public methods

- [`Server$stop()`](#method-Server-stop)

- [`Server$isRunning()`](#method-Server-isRunning)

- [`Server$getStaticPaths()`](#method-Server-getStaticPaths)

- [`Server$setStaticPath()`](#method-Server-setStaticPath)

- [`Server$removeStaticPath()`](#method-Server-removeStaticPath)

- [`Server$getStaticPathOptions()`](#method-Server-getStaticPathOptions)

- [`Server$setStaticPathOption()`](#method-Server-setStaticPathOption)

------------------------------------------------------------------------

### Method [`stop()`](https://rdrr.io/r/base/stop.html)

Stop a running server

#### Usage

    Server$stop()

------------------------------------------------------------------------

### Method `isRunning()`

Check if the server is running

#### Usage

    Server$isRunning()

#### Returns

TRUE if the server is running, FALSE otherwise.

------------------------------------------------------------------------

### Method `getStaticPaths()`

Get the static paths for the server

#### Usage

    Server$getStaticPaths()

#### Returns

A list of
[`staticPath()`](https://rstudio.github.io/httpuv/dev/reference/staticPath.md)
objects.

------------------------------------------------------------------------

### Method `setStaticPath()`

Set a static path for the server

#### Usage

    Server$setStaticPath(..., .list = NULL)

#### Arguments

- `...`:

  Named arguments where each name is the name of the static path and the
  value is the path to the directory to serve. If there already exists a
  static path with the same name, it will be replaced.

- `.list`:

  A named list where each name is the name of the static path and the
  value is the path to the directory to serve. If there already exists a
  static path with the same name, it will be replaced.

#### Examples

    \dontrun{
    # Create a server
    server <- WebServer$new("127.0.0.1", 8080, app = my_app)
    #' # Set a static path
    server$setStaticPath(
      staticPath1 = "path/to/static/files",
      staticPath2 = "another/path/to/static/files"
    )
    }

------------------------------------------------------------------------

### Method `removeStaticPath()`

Remove a static path

#### Usage

    Server$removeStaticPath(path)

#### Arguments

- `path`:

  The name of the static path to remove.

#### Returns

An invisible NULL if the server is running, otherwise it does nothing.

#### Examples

    \dontrun{
    # Create a server
    server <- WebServer$new("127.0.0.1", 8080, app = my_app)
    # Set a static path
    server$setStaticPath(
      staticPath1 = "path/to/static/files",
      staticPath2 = "another/path/to/static/files"
    )
    # Remove a static path
    server$removeStaticPath("staticPath1")
    }

------------------------------------------------------------------------

### Method `getStaticPathOptions()`

Get the static path options for the server

#### Usage

    Server$getStaticPathOptions()

#### Returns

A list of default `staticPathOptions` for the current server. Each
static path will use these options by default, but they can be
overridden for each static path.

------------------------------------------------------------------------

### Method `setStaticPathOption()`

Set one or more static path options

#### Usage

    Server$setStaticPathOption(..., .list = NULL)

#### Arguments

- `...`:

  Named arguments where each name is the name of the static path option
  and the value is the value to set for that option.

- `.list`:

  A named list where each name is the name of the static path option and
  the value is the value to set for that option.

#### Returns

An invisible NULL if the server is running, otherwise it does nothing.

## Examples

``` r

## ------------------------------------------------
## Method `Server$setStaticPath`
## ------------------------------------------------

if (FALSE) { # \dontrun{
# Create a server
server <- WebServer$new("127.0.0.1", 8080, app = my_app)
#' # Set a static path
server$setStaticPath(
  staticPath1 = "path/to/static/files",
  staticPath2 = "another/path/to/static/files"
)
} # }

## ------------------------------------------------
## Method `Server$removeStaticPath`
## ------------------------------------------------

if (FALSE) { # \dontrun{
# Create a server
server <- WebServer$new("127.0.0.1", 8080, app = my_app)
# Set a static path
server$setStaticPath(
  staticPath1 = "path/to/static/files",
  staticPath2 = "another/path/to/static/files"
)
# Remove a static path
server$removeStaticPath("staticPath1")
} # }
```
