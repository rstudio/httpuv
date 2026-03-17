# Find an open TCP port

Finds a random available TCP port for listening on, within a specified
range of ports. The default range of ports to check is 1024 to 49151,
which is the set of TCP User Ports. This function automatically excludes
some ports which are considered unsafe by web browsers.

## Usage

``` r
randomPort(min = 1024L, max = 49151L, host = "127.0.0.1", n = 20)
```

## Arguments

- min:

  Minimum port number.

- max:

  Maximum port number.

- host:

  A string that is a valid IPv4 or IPv6 address that is owned by this
  server, which the application will listen on. `"0.0.0.0"` represents
  all IPv4 addresses and `"::/0"` represents all IPv6 addresses.

- n:

  Number of ports to try before giving up.

## Value

A port that is available to listen on.

## Examples

``` r
if (FALSE) { # \dontrun{
s <- startServer("127.0.0.1", randomPort(), list())
browseURL(paste0("http://127.0.0.1:", s$getPort()))

s$stop()
} # }
```
