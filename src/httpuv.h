#ifndef HTTPUV_HPP
#define HTTPUV_HPP

#include "cpp11.hpp"
#include <Rinternals.h>

using namespace cpp11;

void invokeCppCallback(SEXP data, SEXP callback_xptr);

std::string doEncodeURI(std::string value, bool encodeReserved);
std::string doDecodeURI(std::string value, bool component);

#endif
