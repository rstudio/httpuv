# httpuv: HTTP and WebSocket server library for R

  <!-- badges: start -->
  [![R build status](https://github.com/rstudio/httpuv/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/rstudio/httpuv/actions)
  <!-- badges: end -->

httpuv provides low-level socket and protocol support for handling HTTP and WebSocket requests directly from within R. It uses a multithreaded architecture, where I/O is handled on one thread, and the R callbacks are handled on another.

It is primarily intended as a building block for other packages, rather than making it particularly easy to create complete web applications using httpuv alone. httpuv is built on top of the [libuv](https://github.com/libuv/libuv) and [http-parser](https://github.com/nodejs/http-parser) C libraries, both of which were developed by Joyent, Inc.


## Installing

You can install the stable version from CRAN, or the development version using **remotes**:

```r
# install from CRAN
install.packages("httpuv")

# or if you want to test the development version here
if (!require("remotes")) install.packages("remotes")
remotes::install_github("rstudio/httpuv")
```

Since httpuv contains C code, you'll need to make sure you're set up to install packages with compiled code. Follow the instructions at http://www.rstudio.com/ide/docs/packages/prerequisites

httpuv may optionally be built using a `libuv` system package, which you can install prior to installing the R package. It goes by different names on different package managers: `libuv1-dev` (deb), `libuv-devel` (rpm), `libuv` (brew). Version 1.43 or greater is required. If `libuv` is not found on the system, it will be built from source along with the R package.

## Basic Usage

This is a basic web server that listens on port 8080 and responds to HTTP requests with a web page containing the current system time and the path of the request:

```R
library(httpuv)

s <- startServer(host = "0.0.0.0", port = 8080,
  app = list(
    call = function(req) {
      body <- paste0("Time: ", Sys.time(), "<br>Path requested: ", req$PATH_INFO)
      list(
        status = 200L,
        headers = list('Content-Type' = 'text/html'),
        body = body
      )
    }
  )
)
```

Note that when `host` is 0.0.0.0, it listens on all network interfaces. If `host` is 127.0.0.1, it will only listen to connections from the local host.

The `startServer()` function takes an _app object_, which is a named list with functions that are invoked in response to certain events. In the example above, the list contains a function `call`. This function is invoked when a complete HTTP request is received by the server, and it is passed an environment object `req`, which contains information about HTTP request. `req$PATH_INFO` is the path requested (if the request was for http://127.0.0.1:8080/foo, it would be `"/foo"`).

The `call` function is expected to return a list containing `status`, `headers`, and `body`. That list will be transformed into a HTTP response and sent to the client.

To stop the server:

```R
s$stop()
```

Or, to stop all running httpuv servers:

```R
stopAllServers()
```


### Static paths

A httpuv server application can serve up files on disk. This happens entirely within the I/O thread, so doing so will not block or be blocked by activity in the main R thread.

To serve a path, use `staticPaths` in the app. This will serve the `www/` subdirectory of the current directory (from when `startServer` is called) as the root of the web path:

```R
s <- startServer("0.0.0.0", 8080,
  app = list(
    staticPaths = list("/" = "www/")
  )
)
```

By default, if a file named `index.html` exists in the directory, it will be served when `/` is requested.

`staticPaths` can be combined with `call`. In this example, the web paths `/assets` and `/lib` are served from disk, but requests for any other paths go through the `call` function.

```R
s <- startServer("0.0.0.0", 8080,
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
      # Don't use index.html for /lib
      "/lib" = staticPath("content/lib", indexhtml = FALSE)
    )
  )
)
```


### WebSocket server

httpuv also can handle WebSocket connections. For example, this app acts as a WebSocket echo server:

```R
s <- startServer("127.0.0.1", 8080,
  list(
    onWSOpen = function(ws) {
      # The ws object is a WebSocket object
      cat("Server connection opened.\n")

      ws$onMessage(function(binary, message) {
        cat("Server received message:", message, "\n")
        ws$send(message)
      })
      ws$onClose(function() {
        cat("Server connection closed.\n")
      })
    }
  )
)
```


To test it out, you can connect to it using the [websocket](https://github.com/rstudio/websocket) package (which provides a WebSocket client). You can do this from the same R process or a different one.

```R
ws <- websocket::WebSocket$new("ws://127.0.0.1:8080/")
ws$onMessage(function(event) {
  cat("Client received message:", event$data, "\n")
})

# Wait for a moment before running next line
ws$send("hello world")

# Close client
ws$close()
```

Note that both the httpuv and websocket packages provide a class named `WebSocket`; however, in httpuv, that class acts as a server, and in websocket, it acts as a client. They also have different APIs. For more information about the WebSocket client package, see the [project page](https://github.com/rstudio/websocket).

---


## Debugging builds

httpuv can be built with debugging options enabled. This can be done by uncommenting these lines in src/Makevars, and then installing. The first one enables thread assertions, to ensure that code is running on the correct thread; if not. The second one enables tracing statements: httpuv will print lots of messages when various events occur.

```
PKG_CPPFLAGS += -DDEBUG_THREAD -UNDEBUG
PKG_CPPFLAGS += -DDEBUG_TRACE
```

To install it directly from GitHub with these options, you can use `with_makevars`, like this:

```R
withr::with_makevars(
  c(PKG_CPPFLAGS="-DDEBUG_TRACE -DDEBUG_THREAD -UNDEBUG"), {
    devtools::install_github("rstudio/httpuv")
  }, assignment = "+="
)
```

&copy; 2013-2020 RStudio, Inc.
