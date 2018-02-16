#ifndef HTTPUV_HPP
#define HTTPUV_HPP

#include <Rcpp.h>
#include <Rinternals.h>


void invokeCppCallback(Rcpp::List data, SEXP callback_xptr);

#endif
