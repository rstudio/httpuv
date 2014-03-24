\name{stopDaemonizedServer}
\alias{stopDaemonizedServer}
\title{Stop a running daemonized server in Unix environments}
\usage{
  stopDaemonizedServer(server)
}
\arguments{
  \item{server}{A handle that was previously returned from
  \code{\link{startDaemonizedServer}}.}
}
\description{
  Given a handle that was returned from a previous
  invocation of \code{\link{startDaemonizedServer}}, closes
  all open connections for that server, removes listeners
  in the R event loop and unbinds the port. \strong{Be
  careful not to call \code{stopDaemonizedServer} more than
  once on a handle, as this will cause the R process to
  crash!}
}

