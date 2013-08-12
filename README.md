# httpuv: HTTP and WebSocket server library for R

httpuv provides low-level socket and protocol support for handling
HTTP and WebSocket requests directly from within R. It is primarily intended
as a building block for other packages, rather than making it particularly
easy to create complete web applications using httpuv alone. httpuv is built
on top of the [libuv](https://github.com/joyent/libuv) and [http-parser](https://github.com/joyent/http-parser) C libraries, both of which were developed
by Joyent, Inc.

## Installing

The easiest way to install is with `devtools::install_github`:

```R
# install.packages("devtools")
devtools::install_github("httpuv", "shiny")
```

Since httpuv contains C code, you'll need to make sure you're set up to install packages. 
Follow the instructions at http://www.rstudio.com/ide/docs/packages/prerequisites

<hr/>

&copy; 2013 RStudio, Inc.
