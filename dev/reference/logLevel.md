# Get and set logging level

The logging level for httpuv can be set to report differing levels of
information. Possible logging levels (from least to most information
reported) are: `"OFF"`, `"ERROR"`, `"WARN"`, `"INFO"`, or `"DEBUG"`. The
default level is `ERROR`.

## Usage

``` r
logLevel(level = NULL)
```

## Arguments

- level:

  The logging level. Must be one of `NULL`, `"OFF"`, `"ERROR"`,
  `"WARN"`, `"INFO"`, or `"DEBUG"`. If `NULL` (the default), then this
  function simply returns the current logging level.

## Value

If `level=NULL`, then this returns the current logging level. If `level`
is any other value, then this returns the previous logging level, from
before it is set to the new value.
