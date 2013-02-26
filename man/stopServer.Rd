\name{stopServer}
\alias{stopServer}
\title{Stop a running server}
\usage{
  stopServer(handle)
}
\arguments{
  \item{A}{handle that was previously returned from
  \code{\link{startServer}}.}
}
\description{
  Given a handle that was returned from a previous
  invocation of \code{\link{startServer}}, closes all open
  connections for that server and unbinds the port.
  \strong{Be careful not to call \code{stopServer} more
  than once on a handle, as this will cause the R process
  to crash!}
}

