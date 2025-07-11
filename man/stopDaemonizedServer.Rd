% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/server.R
\name{stopDaemonizedServer}
\alias{stopDaemonizedServer}
\title{Stop a running daemonized server in Unix environments (deprecated)}
\usage{
stopDaemonizedServer(server)
}
\arguments{
\item{server}{A server object that was previously returned from
\code{\link[=startServer]{startServer()}} or \code{\link[=startPipeServer]{startPipeServer()}}.}
}
\description{
This function will be removed in a future release of httpuv. Instead, use
\code{\link[=stopServer]{stopServer()}}.
}
