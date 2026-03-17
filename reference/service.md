# Process requests

Process HTTP requests and WebSocket messages. If there is nothing on R's
call stack – if R is sitting idle at the command prompt – it is not
necessary to call this function, because requests will be handled
automatically. However, if R is executing code, then requests will not
be handled until either the call stack is empty, or this function is
called (or alternatively,
[`later::run_now()`](https://later.r-lib.org/reference/run_now.html) is
called).

## Usage

``` r
service(timeoutMs = ifelse(interactive(), 100, 1000))
```

## Arguments

- timeoutMs:

  Approximate number of milliseconds to run before returning. It will
  return this duration has elapsed. If 0 or Inf, then the function will
  continually process requests without returning unless an error occurs.
  If NA, performs a non-blocking run without waiting.

## Details

In previous versions of httpuv (1.3.5 and below), even if a server
created by
[`startServer()`](https://rstudio.github.io/httpuv/reference/startServer.md)
exists, no requests were serviced unless and until `service` was called.

This function simply calls
[`later::run_now()`](https://later.r-lib.org/reference/run_now.html), so
if your application schedules any
[`later::later()`](https://later.r-lib.org/reference/later.html)
callbacks, they will be invoked.

## Examples

``` r
if (FALSE) { # \dontrun{
while (TRUE) {
  service()
}
} # }
```
