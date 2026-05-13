#ifndef HTTPUV_HPP
#define HTTPUV_HPP

#include <Rinternals.h>
#include "cpp4r.hpp"

using namespace cpp4r;


void invokeCppCallback(SEXP data, SEXP callback_xptr);

std::string doEncodeURI(std::string value, bool encodeReserved);
std::string doDecodeURI(std::string value, bool component);

#endif
