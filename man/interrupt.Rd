% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/httpuv.R
\name{interrupt}
\alias{interrupt}
\title{Interrupt httpuv runloop}
\usage{
interrupt()
}
\description{
Interrupts the currently running httpuv runloop, meaning
\code{\link[=runServer]{runServer()}} or \code{\link[=service]{service()}} will return control back to
the caller and no further tasks will be processed until those methods are
called again. Note that this may cause in-process uploads or downloads to be
interrupted in mid-request.
}
