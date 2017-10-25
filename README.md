# httpuv: HTTP and WebSocket server library for R

[![Travis build status](https://travis-ci.org/rstudio/httpuv.svg?branch=master)](https://travis-ci.org/rstudio/httpuv) [![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/rstudio/httpuv?branch=master&svg=true)](https://ci.appveyor.com/project/rstudio/httpuv)

httpuv provides low-level socket and protocol support for handling
HTTP and WebSocket requests directly from within R. It is primarily intended
as a building block for other packages, rather than making it particularly
easy to create complete web applications using httpuv alone. httpuv is built
on top of the [libuv](https://github.com/joyent/libuv) and [http-parser](https://github.com/joyent/http-parser) C libraries, both of which were developed
by Joyent, Inc.

## Installing

You can install the stable version from CRAN, or the development version using **devtools**:

```r
# install from CRAN
install.packages("httpuv")

# or if you want to test the development version here
if (!require("devtools")) install.packages("devtools")
devtools::install_github("rstudio/httpuv")
```

Since httpuv contains C code, you'll need to make sure you're set up to install packages. 
Follow the instructions at http://www.rstudio.com/ide/docs/packages/prerequisites

---

&copy; 2013-2017 RStudio, Inc.
