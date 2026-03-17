# WebSocket class

A `WebSocket` object represents a single WebSocket connection. The
object can be used to send messages and close the connection, and to
receive notifications when messages are received or the connection is
closed.

## Details

Note that this WebSocket class is different from the one provided by the
package named websocket. This class is meant to be used on the server
side, whereas the one in the websocket package is to be used as a
client. The WebSocket class in httpuv has an older API than the one in
the websocket package.

WebSocket objects should never be created directly. They are obtained by
passing an `onWSOpen` function to
[`startServer()`](https://rstudio.github.io/httpuv/reference/startServer.md).

## Public fields

- `handle`:

  The server handle

- `messageCallbacks`:

  A list of callback functions that will be invoked when a message is
  received on this connection.

- `closeCallbacks`:

  A list of callback functions that will be invoked when the connection
  is closed.

- `request`:

  The Rook request environment that opened the connection. This can be
  used to inspect HTTP headers, for example.

## Methods

### Public methods

- [`WebSocket$new()`](#method-WebSocket-new)

- [`WebSocket$onMessage()`](#method-WebSocket-onMessage)

- [`WebSocket$onClose()`](#method-WebSocket-onClose)

- [`WebSocket$send()`](#method-WebSocket-send)

- [`WebSocket$close()`](#method-WebSocket-close)

- [`WebSocket$clone()`](#method-WebSocket-clone)

------------------------------------------------------------------------

### Method `new()`

Initializes a new WebSocket object.

#### Usage

    WebSocket$new(handle, req)

#### Arguments

- `handle`:

  An C++ WebSocket handle.

- `req`:

  The Rook request environment that opened the connection.

------------------------------------------------------------------------

### Method `onMessage()`

Registers a callback function that will be invoked whenever a message is
received on this connection.

#### Usage

    WebSocket$onMessage(func)

#### Arguments

- `func`:

  The callback function to be registered. The callback function will be
  invoked with two arguments. The first argument is `TRUE` if the
  message is binary and `FALSE` if it is text. The second argument is
  either a raw vector (if the message is binary) or a character vector.

------------------------------------------------------------------------

### Method `onClose()`

Registers a callback function that will be invoked when the connection
is closed.

#### Usage

    WebSocket$onClose(func)

#### Arguments

- `func`:

  The callback function to be registered.

------------------------------------------------------------------------

### Method `send()`

Begins sending the given message over the websocket.

#### Usage

    WebSocket$send(message)

#### Arguments

- `message`:

  Either a raw vector, or a single-element character vector that is
  encoded in UTF-8.

------------------------------------------------------------------------

### Method [`close()`](https://rdrr.io/r/base/connections.html)

Closes the websocket connection

#### Usage

    WebSocket$close(code = 1000L, reason = "")

#### Arguments

- `code`:

  An integer that indicates the [WebSocket close
  code](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/close#code).

- `reason`:

  A concise human-readable prose [explanation for the
  closure](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/close#reason).

------------------------------------------------------------------------

### Method `clone()`

The objects of this class are cloneable with this method.

#### Usage

    WebSocket$clone(deep = FALSE)

#### Arguments

- `deep`:

  Whether to make a deep clone.

## Examples

``` r

if (FALSE) { # \dontrun{
# A WebSocket echo server that listens on port 8080
startServer("0.0.0.0", 8080,
  list(
    onHeaders = function(req) {
      # Print connection headers
      cat(capture.output(str(as.list(req))), sep = "\n")
    },
    onWSOpen = function(ws) {
      cat("Connection opened.\n")

      ws$onMessage(function(binary, message) {
        cat("Server received message:", message, "\n")
        ws$send(message)
      })
      ws$onClose(function() {
        cat("Connection closed.\n")
      })

    }
  )
)
} # }
```
