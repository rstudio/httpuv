\name{WebSocket}
\alias{WebSocket}
\title{WebSocket object}
\description{
  An object that represents a single WebSocket connection.
  The object can be used to send messages and close the
  connection, and to receive notifications when messages
  are received or the connection is closed.
}
\details{
  WebSocket objects should never be created directly. They
  are obtained by passing an \code{onWSOpen} function to
  \code{\link{startServer}}.

  \strong{Fields}

  \describe{ \item{\code{request}}{ The Rook request
  environment that opened the connection. This can be used
  to inspect HTTP headers, for example.  } }

  \strong{Methods}

  \describe{ \item{\code{onMessage(func)}}{ Registers a
  callback function that will be invoked whenever a message
  is received on this connection. The callback function
  will be invoked with two arguments. The first argument is
  \code{TRUE} if the message is binary and \code{FALSE} if
  it is text. The second argument is either a raw vector
  (if the message is binary) or a character vector.  }
  \item{\code{onClose(func)}}{ Registers a callback
  function that will be invoked when the connection is
  closed.  } \item{\code{send(message)}}{ Begins sending
  the given message over the websocket. The message must be
  either a raw vector, or a single-element character vector
  that is encoded in UTF-8.  } \item{\code{close()}}{
  Closes the websocket connection.  } }
}

