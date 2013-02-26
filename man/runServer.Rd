\name{runServer}
\alias{runServer}
\title{Run a server}
\usage{
  runServer(host, port, app,
    interruptIntervalMs = ifelse(interactive(), 100, 1000))
}
\arguments{
  \item{host}{A string that is a valid IPv4 address that is
  owned by this server, or \code{"0.0.0.0"} to listen on
  all IP addresses.}

  \item{port}{A number or integer that indicates the server
  port that should be listened on. Note that on most
  Unix-like systems including Linux and Mac OS X, port
  numbers smaller than 1025 require root privileges.}

  \item{app}{A collection of functions that define your
  application. See Details.}

  \item{interruptIntervalMs}{How often to check for
  interrupt. The default should be appropriate for most
  situations.}
}
\description{
  This is a convenience function that provides a simple way
  to call \code{\link{startServer}}, \code{\link{service}},
  and \code{\link{stopServer}} in the correct sequence. It
  does not return unless interrupted or an error occurs.
}
\details{
  If you have multiple hosts and/or ports to listen on,
  call the individual functions instead of
  \code{runServer}.
}
\seealso{
  \code{\link{startServer}}, \code{\link{service}},
  \code{\link{stopServer}}
}

