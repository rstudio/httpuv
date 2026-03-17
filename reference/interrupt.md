# Interrupt httpuv runloop

Interrupts the currently running httpuv runloop, meaning
[`runServer()`](https://rstudio.github.io/httpuv/reference/runServer.md)
or [`service()`](https://rstudio.github.io/httpuv/reference/service.md)
will return control back to the caller and no further tasks will be
processed until those methods are called again. Note that this may cause
in-process uploads or downloads to be interrupted in mid-request.

## Usage

``` r
interrupt()
```
