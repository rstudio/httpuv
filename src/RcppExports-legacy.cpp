// This file is unfortunately needed because Shiny 1.0.5 and below included a
// copy of httpuv::decodeURIComponent. With httpuv 1.3.5 and below, this was
// defined as:
// function (value) {
//   .Call("httpuv_decodeURIComponent", PACKAGE = "httpuv", value)
// }
//
// However, later versions of httpuv were built with Rcpp 0.12.12, which
// changed the name of the C function to "_httpuv_decodeURIComponent". The
// result is that if Shiny 1.0.5 was built with httpuv 1.3.5 but run with a
// newer version of httpuv, it would try to call out to a nonexistent C
// function named "httpuv_decodeURIComponent". The function below is a copy of
// _httpuv_decodeURIComponent, but with the old name. It preserves backward
// compatibility, so that Shiny 1.0.5 (and older) built with httpuv 1.3.5 (and
// older) would be able to run with newer versions of httpuv.
//
// This issue was fixed with Shiny 1.0.6:
// https://github.com/rstudio/shiny/commit/dc18b20e5a9b8b2b74a49f44c920f5159801d147
//
// It will probably be safe to remove this after Shiny 1.0.6 has been out for
// a year or so.

#include <Rcpp.h>

using namespace Rcpp;

std::vector<std::string> decodeURIComponent(std::vector<std::string> value);
RcppExport SEXP httpuv_decodeURIComponent(SEXP valueSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< std::vector<std::string> >::type value(valueSEXP);
    rcpp_result_gen = Rcpp::wrap(decodeURIComponent(value));
    return rcpp_result_gen;
END_RCPP
}
