# Convert raw vector to Base64-encoded string

Converts a raw vector to its Base64 encoding as a single-element
character vector.

## Usage

``` r
rawToBase64(x)
```

## Arguments

- x:

  A raw vector.

## Examples

``` r
set.seed(100)
result <- rawToBase64(as.raw(runif(19, min=0, max=256)))
stopifnot(identical(result, "TkGNDnd7z16LK5/hR2bDqzRbXA=="))
```
