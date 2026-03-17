# Create a staticPath object

The `staticPath` function creates a `staticPath` object. Note that if
any of the arguments (other than `path`) are `NULL`, then that means
that for this particular static path, it should inherit the behavior
from the staticPathOptions set for the application as a whole.

## Usage

``` r
staticPath(
  path,
  indexhtml = NULL,
  fallthrough = NULL,
  html_charset = NULL,
  headers = NULL,
  validation = NULL
)

excludeStaticPath()
```

## Arguments

- path:

  The local path.

- indexhtml:

  If an index.html file is present, should it be served up when the
  client requests the static path or any subdirectory?

- fallthrough:

  With the default value, `FALSE`, if a request is made for a file that
  doesn't exist, then httpuv will immediately send a 404 response from
  the background I/O thread, without needing to call back into the main
  R thread. This offers the best performance. If the value is `TRUE`,
  then instead of sending a 404 response, httpuv will call the
  application's `call` function, and allow it to handle the request.

- html_charset:

  When HTML files are served, the value that will be provided for
  `charset` in the Content-Type header. For example, with the default
  value, `"utf-8"`, the header is
  `Content-Type: text/html; charset=utf-8`. If `""` is used, then no
  `charset` will be added in the Content-Type header.

- headers:

  Additional headers and values that will be included in the response.

- validation:

  An optional validation pattern. Presently, the only type of validation
  supported is an exact string match of a header. For example, if
  `validation` is `'"abc" = "xyz"'`, then HTTP requests must have a
  header named `abc` (case-insensitive) with the value `xyz`
  (case-sensitive). If a request does not have a matching header, than
  httpuv will give a 403 Forbidden response. If the `character(0)` (the
  default), then no validation check will be performed.

## Details

The `excludeStaticPath` function tells the application to ignore a
particular path for static serving. This is useful when you want to
include a path for static serving (like `"/"`) but then exclude a
subdirectory of it (like `"/dynamic"`) so that the subdirectory will
always be passed to the R code for handling requests.
`excludeStaticPath` can be used not only for directories; it can also
exclude specific files.

## See also

[`staticPathOptions()`](https://rstudio.github.io/httpuv/reference/staticPathOptions.md).
