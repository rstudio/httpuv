# Apply the value of .Random.seed to R's internal RNG state

This function is needed in unusual cases where a C++ function calls an R
function which sets the value of `.Random.seed`. This function should be
called at the end of the R function to ensure that the new value
`.Random.seed` is preserved. Otherwise, Rcpp may overwrite it with a
previous value.

## Usage

``` r
getRNGState()
```
