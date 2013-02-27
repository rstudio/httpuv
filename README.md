# httpuv: HTTP and WebSocket server library for R

httpuv provides low-level socket and protocol support for handling
HTTP and WebSocket requests directly from within R. It is primarily intended
as a building block for other packages, rather than making it particularly
easy to create complete web applications using httpuv alone. httpuv is built
on top of the [libuv](https://github.com/joyent/libuv) and [http-parser](https://github.com/joyent/http-parser) C libraries, both of which were developed
by Joyent, Inc.

## Installing

The [Rcpp](http://cran.r-project.org/web/packages/Rcpp/index.html) package is required.

Windows users also need to make sure the correct version of [RTools](http://cran.r-project.org/bin/windows/Rtools/) is installed and in the system path. 

```
git clone https://github.com/rstudio/httpuv
cd httpuv
git submodule update --init
R CMD INSTALL .
```

<hr/>

&copy; 2013 RStudio, Inc.
