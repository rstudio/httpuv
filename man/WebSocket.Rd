% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/httpuv.R
\name{WebSocket}
\alias{WebSocket}
\title{WebSocket class}
\description{
A \code{WebSocket} object represents a single WebSocket connection. The
object can be used to send messages and close the connection, and to receive
notifications when messages are received or the connection is closed.
}
\details{
Note that this WebSocket class is different from the one provided by the
package named websocket. This class is meant to be used on the server side,
whereas the one in the websocket package is to be used as a client. The
WebSocket class in httpuv has an older API than the one in the websocket
package.

WebSocket objects should never be created directly. They are obtained by
passing an \code{onWSOpen} function to \code{\link[=startServer]{startServer()}}.
}
\examples{

\dontrun{
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
}
}
\section{Public fields}{
\if{html}{\out{<div class="r6-fields">}}
\describe{
\item{\code{handle}}{The server handle}

\item{\code{messageCallbacks}}{A list of callback functions that will be invoked
when a message is received on this connection.}

\item{\code{closeCallbacks}}{A list of callback functions that will be invoked
when the connection is closed.}

\item{\code{request}}{The Rook request environment that opened the connection.
This can be used to inspect HTTP headers, for example.}
}
\if{html}{\out{</div>}}
}
\section{Methods}{
\subsection{Public methods}{
\itemize{
\item \href{#method-WebSocket-new}{\code{WebSocket$new()}}
\item \href{#method-WebSocket-onMessage}{\code{WebSocket$onMessage()}}
\item \href{#method-WebSocket-onClose}{\code{WebSocket$onClose()}}
\item \href{#method-WebSocket-send}{\code{WebSocket$send()}}
\item \href{#method-WebSocket-close}{\code{WebSocket$close()}}
\item \href{#method-WebSocket-clone}{\code{WebSocket$clone()}}
}
}
\if{html}{\out{<hr>}}
\if{html}{\out{<a id="method-WebSocket-new"></a>}}
\if{latex}{\out{\hypertarget{method-WebSocket-new}{}}}
\subsection{Method \code{new()}}{
Initializes a new WebSocket object.
\subsection{Usage}{
\if{html}{\out{<div class="r">}}\preformatted{WebSocket$new(handle, req)}\if{html}{\out{</div>}}
}

\subsection{Arguments}{
\if{html}{\out{<div class="arguments">}}
\describe{
\item{\code{handle}}{An C++ WebSocket handle.}

\item{\code{req}}{The Rook request environment that opened the connection.}
}
\if{html}{\out{</div>}}
}
}
\if{html}{\out{<hr>}}
\if{html}{\out{<a id="method-WebSocket-onMessage"></a>}}
\if{latex}{\out{\hypertarget{method-WebSocket-onMessage}{}}}
\subsection{Method \code{onMessage()}}{
Registers a callback function that will be invoked whenever a message is
received on this connection.
\subsection{Usage}{
\if{html}{\out{<div class="r">}}\preformatted{WebSocket$onMessage(func)}\if{html}{\out{</div>}}
}

\subsection{Arguments}{
\if{html}{\out{<div class="arguments">}}
\describe{
\item{\code{func}}{The callback function to be registered. The callback function will be invoked with
two arguments. The first argument is \code{TRUE} if the message is binary
and \code{FALSE} if it is text. The second argument is either a raw
vector (if the message is binary) or a character vector.}
}
\if{html}{\out{</div>}}
}
}
\if{html}{\out{<hr>}}
\if{html}{\out{<a id="method-WebSocket-onClose"></a>}}
\if{latex}{\out{\hypertarget{method-WebSocket-onClose}{}}}
\subsection{Method \code{onClose()}}{
Registers a callback function that will be invoked when the connection is
closed.
\subsection{Usage}{
\if{html}{\out{<div class="r">}}\preformatted{WebSocket$onClose(func)}\if{html}{\out{</div>}}
}

\subsection{Arguments}{
\if{html}{\out{<div class="arguments">}}
\describe{
\item{\code{func}}{The callback function to be registered.}
}
\if{html}{\out{</div>}}
}
}
\if{html}{\out{<hr>}}
\if{html}{\out{<a id="method-WebSocket-send"></a>}}
\if{latex}{\out{\hypertarget{method-WebSocket-send}{}}}
\subsection{Method \code{send()}}{
Begins sending the given message over the websocket.
\subsection{Usage}{
\if{html}{\out{<div class="r">}}\preformatted{WebSocket$send(message)}\if{html}{\out{</div>}}
}

\subsection{Arguments}{
\if{html}{\out{<div class="arguments">}}
\describe{
\item{\code{message}}{Either a raw vector, or a single-element character
vector that is encoded in UTF-8.}
}
\if{html}{\out{</div>}}
}
}
\if{html}{\out{<hr>}}
\if{html}{\out{<a id="method-WebSocket-close"></a>}}
\if{latex}{\out{\hypertarget{method-WebSocket-close}{}}}
\subsection{Method \code{close()}}{
Closes the websocket connection
\subsection{Usage}{
\if{html}{\out{<div class="r">}}\preformatted{WebSocket$close(code = 1000L, reason = "")}\if{html}{\out{</div>}}
}

\subsection{Arguments}{
\if{html}{\out{<div class="arguments">}}
\describe{
\item{\code{code}}{An integer that indicates the \href{https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/close#code}{WebSocket close code}.}

\item{\code{reason}}{A concise human-readable prose \href{https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/close#reason}{explanation for the closure}.}
}
\if{html}{\out{</div>}}
}
}
\if{html}{\out{<hr>}}
\if{html}{\out{<a id="method-WebSocket-clone"></a>}}
\if{latex}{\out{\hypertarget{method-WebSocket-clone}{}}}
\subsection{Method \code{clone()}}{
The objects of this class are cloneable with this method.
\subsection{Usage}{
\if{html}{\out{<div class="r">}}\preformatted{WebSocket$clone(deep = FALSE)}\if{html}{\out{</div>}}
}

\subsection{Arguments}{
\if{html}{\out{<div class="arguments">}}
\describe{
\item{\code{deep}}{Whether to make a deep clone.}
}
\if{html}{\out{</div>}}
}
}
}
