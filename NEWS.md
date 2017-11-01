httpuv 1.3.5.9002
=================

* Upgraded to libuv 1.15.0. ([#91](https://github.com/rstudio/httpuv/pull/91))

* Upgraded to http-parser 2.7.1. ([#93](https://github.com/rstudio/httpuv/pull/93))

* httpuv can now handle request callbacks asynchronously. ([#80](https://github.com/rstudio/httpuv/pull/80))

* Fixed [#72](https://github.com/rstudio/httpuv/issues/72): httpuv previously did not close connections that had the `Connection: close` header, or were HTTP 1.0 (without `Connection: keep-alive`). ([#99](https://github.com/rstudio/httpuv/pull/99))

* Let Rcpp handle symbol registration. ([#85](https://github.com/rstudio/httpuv/pull/85))

* Hide internal symbols from shared library on supported platforms. This reduces the risk of conflicts with other packages bundling libuv. ([#85](https://github.com/rstudio/httpuv/pull/85))

* Fixed [#86](https://github.com/rstudio/httpuv/issues/86): `encodeURI()` gave incorrect output for non-ASCII characters. ([#87](https://github.com/rstudio/httpuv/pull/87))


httpuv 1.3.5
============

* Added function `getRNGState`.


httpuv 1.3.3
============

* Error messages are now sent as UTF-8.

* httpuv no longer adds a Content-Length header if one has already been provided. This is for Shiny issue #876.


httpuv 1.3.2
============

* Add `encodeURI`, `encodeURIComponent`, `decodeURI`, and `decodeURIComponent` functions.

* Compatibility with Rook middleware reference classes.


httpuv 1.3.1
============

* Fix bug where websocket headers split over multiple packets would cause the payload to be parsed incorrectly.


httpuv 1.3.0
============

* Add experimental support for running httpuv servers in the background (see `?startDaemonizedServer` and `?stopDaemonizedServer`). Many thanks to Héctor Corrada Bravo for the contribution!


httpuv 1.2.3
============

* Require Rcpp 0.11.0. The absence of this requirement made it too easy for Windows and Mac users with Rcpp 0.10 already installed to grab httpuv 1.2.2 binaries from CRAN, which are built against Rcpp 0.11, causing bad crashes due to Rcpp's linkage changes.


httpuv 1.2.2
============

* Export base64 encoding function `rawToBase64`.

* Compatibility work for Rcpp 0.11.0.


httpuv 1.2.1
============

* Solaris 10 compatibility fixes (courtesy of Dr. Brian Ripley).


httpuv 1.2.0
============


* Fix IE10 websocket handshake failure.

* Implement hixie-76 version of WebSocket protocol, for Safari 4 and QtWebKit.


httpuv 1.1.0
============


* Fix issue #8: Bug in concurrent uploads.

* Add `interrupt()` function for stopping the runloop.

* Add REMOTE_ADDR and REMOTE_PORT to request environment.

* Switch from git submodules to git subtree; much easier installation of development builds.

* Upgrade to libuv v0.10.13.

* Fix issue #13: Segfault on successful retry of server creation.


httpuv 1.0.6.3
==============

* Greatly improved stability under heavy load by ignoring SIGPIPE.


httpuv 1.0.6.2
==============

* Work properly with `body=c(file="foo")`. Previously this only worked if body was a list, not a character vector.

* R CMD INSTALL will do `git submodule update --init` if necessary.

* When `onHeaders()` callback returned a body, httpuv was not properly short-circuiting the request.

* Ignore SIGPIPE permanently. This was still causing crashes under heavy real-world traffic.


httpuv 1.0.6.1
==============

* Make request available on websocket object.


httpuv 1.0.6
============

* Support listening on pipes (Unix domain sockets have been tested, Windows named pipes have not been tested but may work).

* Fix crash on CentOS 6.4 due to weird interaction with OpenSSL.


httpuv 1.0.5
============

* Initial release.
