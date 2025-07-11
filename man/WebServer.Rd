% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/server.R
\name{WebServer}
\alias{WebServer}
\title{WebServer class}
\description{
This class represents a web server running one application. Multiple servers
can be running at the same time.
}
\examples{

## ------------------------------------------------
## Method `WebServer$new`
## ------------------------------------------------

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
}
\seealso{
\code{\link[=Server]{Server()}} and \code{\link[=PipeServer]{PipeServer()}}.
}
\keyword{internal}
\section{Super class}{
\code{\link[httpuv:Server]{httpuv::Server}} -> \code{WebServer}
}
\section{Methods}{
\subsection{Public methods}{
\itemize{
\item \href{#method-WebServer-new}{\code{WebServer$new()}}
\item \href{#method-WebServer-getHost}{\code{WebServer$getHost()}}
\item \href{#method-WebServer-getPort}{\code{WebServer$getPort()}}
}
}
\if{html}{\out{
<details><summary>Inherited methods</summary>
<ul>
<li><span class="pkg-link" data-pkg="httpuv" data-topic="Server" data-id="getStaticPathOptions"><a href='../../httpuv/html/Server.html#method-Server-getStaticPathOptions'><code>httpuv::Server$getStaticPathOptions()</code></a></span></li>
<li><span class="pkg-link" data-pkg="httpuv" data-topic="Server" data-id="getStaticPaths"><a href='../../httpuv/html/Server.html#method-Server-getStaticPaths'><code>httpuv::Server$getStaticPaths()</code></a></span></li>
<li><span class="pkg-link" data-pkg="httpuv" data-topic="Server" data-id="isRunning"><a href='../../httpuv/html/Server.html#method-Server-isRunning'><code>httpuv::Server$isRunning()</code></a></span></li>
<li><span class="pkg-link" data-pkg="httpuv" data-topic="Server" data-id="removeStaticPath"><a href='../../httpuv/html/Server.html#method-Server-removeStaticPath'><code>httpuv::Server$removeStaticPath()</code></a></span></li>
<li><span class="pkg-link" data-pkg="httpuv" data-topic="Server" data-id="setStaticPath"><a href='../../httpuv/html/Server.html#method-Server-setStaticPath'><code>httpuv::Server$setStaticPath()</code></a></span></li>
<li><span class="pkg-link" data-pkg="httpuv" data-topic="Server" data-id="setStaticPathOption"><a href='../../httpuv/html/Server.html#method-Server-setStaticPathOption'><code>httpuv::Server$setStaticPathOption()</code></a></span></li>
<li><span class="pkg-link" data-pkg="httpuv" data-topic="Server" data-id="stop"><a href='../../httpuv/html/Server.html#method-Server-stop'><code>httpuv::Server$stop()</code></a></span></li>
</ul>
</details>
}}
\if{html}{\out{<hr>}}
\if{html}{\out{<a id="method-WebServer-new"></a>}}
\if{latex}{\out{\hypertarget{method-WebServer-new}{}}}
\subsection{Method \code{new()}}{
Initialize a new WebServer object

Create a new \code{WebServer} object. \code{app} is an httpuv application
object as described in \code{\link[=startServer]{startServer()}}.
\subsection{Usage}{
\if{html}{\out{<div class="r">}}\preformatted{WebServer$new(host, port, app, quiet = FALSE)}\if{html}{\out{</div>}}
}

\subsection{Arguments}{
\if{html}{\out{<div class="arguments">}}
\describe{
\item{\code{host}}{The host name or IP address to bind the server to.}

\item{\code{port}}{The port number to bind the server to.}

\item{\code{app}}{An httpuv application object as described in \code{\link[=startServer]{startServer()}}.}

\item{\code{quiet}}{If TRUE, suppresses output from the server.}
}
\if{html}{\out{</div>}}
}
\subsection{Returns}{
A new \code{WebServer} object.
}
\subsection{Examples}{
\if{html}{\out{<div class="r example copy">}}
\preformatted{\dontrun{
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
}
\if{html}{\out{</div>}}

}

}
\if{html}{\out{<hr>}}
\if{html}{\out{<a id="method-WebServer-getHost"></a>}}
\if{latex}{\out{\hypertarget{method-WebServer-getHost}{}}}
\subsection{Method \code{getHost()}}{
Get the host name or IP address of the server
\subsection{Usage}{
\if{html}{\out{<div class="r">}}\preformatted{WebServer$getHost()}\if{html}{\out{</div>}}
}

\subsection{Returns}{
The host name or IP address that the server is bound to.
}
}
\if{html}{\out{<hr>}}
\if{html}{\out{<a id="method-WebServer-getPort"></a>}}
\if{latex}{\out{\hypertarget{method-WebServer-getPort}{}}}
\subsection{Method \code{getPort()}}{
Get the port number of the server
\subsection{Usage}{
\if{html}{\out{<div class="r">}}\preformatted{WebServer$getPort()}\if{html}{\out{</div>}}
}

\subsection{Returns}{
The port number that the server is bound to.
}
}
}
