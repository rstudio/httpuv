#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <map>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <cstring>
#include <Rinternals.h>
// Undo R's length() macro so std::string::length() and STL internals work.
#ifdef length
# undef length
#endif
#include "optional.h"
#include "thread.h"
#include "timegm.h"

// ============================================================================
// RAII wrapper for R_PreserveObject / R_ReleaseObject
// ============================================================================

struct RProtectedSEXP {
  SEXP sexp;
  explicit RProtectedSEXP(SEXP s = R_NilValue) : sexp(s) {
    R_PreserveObject(sexp);
  }
  ~RProtectedSEXP() {
    R_ReleaseObject(sexp);
  }
  operator SEXP() const { return sexp; }
  RProtectedSEXP(const RProtectedSEXP&) = delete;
  RProtectedSEXP& operator=(const RProtectedSEXP&) = delete;
};

// ============================================================================
// Type conversion helpers (R C API equivalents of cpp4r::as_cpp / as_sexp)
// ============================================================================

template<typename T>
inline T as_cpp(SEXP x);

template<>
inline bool as_cpp<bool>(SEXP x) {
  if (TYPEOF(x) == LGLSXP) return LOGICAL(x)[0] != 0;
  if (TYPEOF(x) == INTSXP) return INTEGER(x)[0] != 0;
  if (TYPEOF(x) == REALSXP) return REAL(x)[0] != 0;
  // Fallback: coerce
  SEXP coerced = PROTECT(Rf_coerceVector(x, LGLSXP));
  bool result = LOGICAL(coerced)[0] != 0;
  UNPROTECT(1);
  return result;
}

template<>
inline int as_cpp<int>(SEXP x) {
  if (TYPEOF(x) == INTSXP) return INTEGER(x)[0];
  if (TYPEOF(x) == REALSXP) return (int)REAL(x)[0];
  if (TYPEOF(x) == LGLSXP) return LOGICAL(x)[0];
  // Fallback: coerce (e.g. CHARSXP)
  SEXP coerced = PROTECT(Rf_coerceVector(x, INTSXP));
  int result = INTEGER(coerced)[0];
  UNPROTECT(1);
  return result;
}

template<>
inline std::string as_cpp<std::string>(SEXP x) {
  return std::string(CHAR(STRING_ELT(x, 0)));
}

template<>
inline std::vector<std::string> as_cpp<std::vector<std::string>>(SEXP x) {
  R_xlen_t n = Rf_xlength(x);
  std::vector<std::string> result;
  result.reserve(n);
  for (R_xlen_t i = 0; i < n; ++i)
    result.push_back(std::string(CHAR(STRING_ELT(x, i))));
  return result;
}

inline SEXP as_sexp(bool x) {
  return Rf_ScalarLogical(x ? TRUE : FALSE);
}

inline SEXP as_sexp(const std::string& x) {
  return Rf_mkString(x.c_str());
}

inline SEXP as_sexp(const std::vector<std::string>& x) {
  SEXP result = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t)x.size()));
  for (size_t i = 0; i < x.size(); ++i)
    SET_STRING_ELT(result, (R_xlen_t)i, Rf_mkChar(x[i].c_str()));
  UNPROTECT(1);
  return result;
}

inline SEXP as_sexp(const std::vector<uint8_t>& x) {
  SEXP result = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t)x.size()));
  if (!x.empty()) memcpy(RAW(result), x.data(), x.size());
  UNPROTECT(1);
  return result;
}

// ============================================================================
// List helper functions
// ============================================================================

// Helper function to get element by name from a list SEXP
inline SEXP get_list_element(SEXP lst, const char* name) {
  SEXP names = Rf_getAttrib(lst, R_NamesSymbol);
  if (names == R_NilValue) return R_NilValue;
  for (R_xlen_t i = 0; i < Rf_xlength(lst); i++) {
    if (strcmp(CHAR(STRING_ELT(names, i)), name) == 0) {
      return VECTOR_ELT(lst, i);
    }
  }
  return R_NilValue;
}

// Helper function to check if list contains element by name
inline bool list_contains_element(SEXP lst, const char* name) {
  SEXP names = Rf_getAttrib(lst, R_NamesSymbol);
  if (names == R_NilValue) return false;
  for (R_xlen_t i = 0; i < Rf_xlength(lst); i++) {
    if (strcmp(CHAR(STRING_ELT(names, i)), name) == 0) {
      return true;
    }
  }
  return false;
}

// ============================================================================

// A callback for deleting objects on the main thread using later(). This is
// needed when the object holds R memory that must only be released on the main
// thread.
template <typename T>
void deleter_main(void* obj) {
  ASSERT_MAIN_THREAD()
  // later() passes a void* to the callback, so we have to cast it.
  T* typed_obj = reinterpret_cast<T*>(obj);

  try {
    delete typed_obj;
  } catch (...) {}
}

// Does the same as deleter_main, but checks that it's running on the
// background thread (when thread debugging is enabled).
template <typename T>
void deleter_background(void* obj) {
  ASSERT_BACKGROUND_THREAD()
  T* typed_obj = reinterpret_cast<T*>(obj);

  try {
    delete typed_obj;
  } catch (...) {}
}

// It's not safe to call REprintf from the background thread but we need some
// way to output error messages. R CMD check does not it if the code uses the
// symbols stdout, stderr, and printf, so this function is a way to avoid
// those. It's to calling `fprintf(stderr, ...)`.
inline void err_printf(const char *fmt, ...) {
  const size_t max_size = 4096;
  char buf[max_size];

  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf, max_size, fmt, args);
  va_end(args);

  if (n == -1)
    return;

  ssize_t res = write(STDERR_FILENO, buf, n);
  // This is here simply to avoid a warning about "ignoring return value" of the
  // write(), or "variable 'res' set but not used" on some compilers.
  if (res) res += 0;
  return;
}


// ============================================================================
// Logging
// ============================================================================

enum LogLevel {
  LOG_OFF,
  LOG_ERROR,
  LOG_WARN,
  LOG_INFO,
  LOG_DEBUG
};

void debug_log(const std::string& msg, LogLevel level);

// ============================================================================


// Indexing into an empty vector causes assertion failures on some platforms
template <typename T>
T* safe_vec_addr(std::vector<T>& vec) {
  return vec.size() ? &vec[0] : NULL;
}

// Indexing into an empty vector causes assertion failures on some platforms
inline const char* safe_str_addr(const std::string& str) {
  return str.size() ? &str[0] : NULL;
}

inline std::string to_lower(const std::string& str) {
  std::string lowered = str;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), tolower);
  return lowered;
}

template <typename T>
std::string toString(T x) {
  std::stringstream ss;
  ss << x;
  return ss.str();
}

// This is used for converting a named R list (SEXP) to a std::map.
template <typename T1>
std::map<std::string, T1> toMap(SEXP x) {
  ASSERT_MAIN_THREAD()

  std::map<std::string, T1> strmap;

  if (Rf_xlength(x) == 0) {
    return strmap;
  }

  SEXP names_sexp = Rf_getAttrib(x, R_NamesSymbol);
  if (names_sexp == R_NilValue || Rf_xlength(names_sexp) == 0) {
    throw std::runtime_error("Error converting R object to map<string, T>: vector does not have names.");
  }

  for (R_xlen_t i = 0; i < Rf_xlength(x); i++) {
    std::string name = std::string(CHAR(STRING_ELT(names_sexp, i)));
    T1          value = as_cpp<T1>(VECTOR_ELT(x, i));
    if (name == "") {
      throw std::runtime_error("Error converting R object to map<string, T>: element has empty name.");
    }

    strmap.insert(
      std::pair<std::string, T1>(name, value)
    );
  }

  return strmap;
}

// A wrapper for as_cpp. If the R value is NULL, this returns nullopt;
// otherwise it returns the usual value that as_cpp returns, wrapped in
// std::experimental::optional<T>.
template <typename T1>
std::experimental::optional<T1> optional_as(SEXP value) {
  if (value == R_NilValue) {
    return std::experimental::nullopt;
  }
  return std::experimental::optional<T1>( as_cpp<T1>(value) );
}

// A wrapper for as_sexp. If the C++ value is missing, this returns the
// R value NULL; otherwise it returns the usual value that as_sexp returns, after
// unwrapping from the std::experimental::optional<T>.
template <typename T>
SEXP optional_wrap(std::experimental::optional<T> value) {
  if (!value.has_value()) {
    return R_NilValue;
  }
  return as_sexp(*value);
}


// Conversion functions for ResponseHeaders (vector of pairs of strings).
// Since the ResponseHeaders typedef is in constants.h and this file doesn't
// include constants.h, we'll define them using the actual vector type instead
// of the ResponseHeaders typedef.

inline std::vector<std::pair<std::string, std::string> > response_headers_from_sexp(SEXP x) {
  ASSERT_MAIN_THREAD()
  SEXP names_sexp = Rf_getAttrib(x, R_NamesSymbol);

  if (names_sexp == R_NilValue) {
    throw std::runtime_error("All values must be named.");
  }

  std::vector<std::pair<std::string, std::string> > result;

  for (R_xlen_t i = 0; i < Rf_xlength(x); i++) {
    std::string name = std::string(CHAR(STRING_ELT(names_sexp, i)));
    if (name == "") {
      throw std::runtime_error("All values must be named.");
    }

    std::string value = std::string(CHAR(STRING_ELT(x, i)));

    result.push_back(std::make_pair(name, value));
  }

  return result;
}

inline SEXP response_headers_to_sexp(const std::vector<std::pair<std::string, std::string> > &x) {
  ASSERT_MAIN_THREAD()

  R_xlen_t n = (R_xlen_t)x.size();
  SEXP result = PROTECT(Rf_allocVector(STRSXP, n));
  SEXP names  = PROTECT(Rf_allocVector(STRSXP, n));

  for (size_t i = 0; i < x.size(); i++) {
    SET_STRING_ELT(names,  (R_xlen_t)i, Rf_mkChar(x[i].first.c_str()));
    SET_STRING_ELT(result, (R_xlen_t)i, Rf_mkChar(x[i].second.c_str()));
  }

  Rf_setAttrib(result, R_NamesSymbol, names);
  UNPROTECT(2);
  return result;
}


// Return a date string in the format required for the HTTP Date header. For
// example: "Wed, 21 Oct 2015 07:28:00 GMT"
inline std::string http_date_string(const time_t& t) {
  struct tm timeptr;
  #ifdef _WIN32
  gmtime_s(&timeptr, &t);
  #else
  gmtime_r(&t, &timeptr);
  #endif

  std::string day_name;
  switch(timeptr.tm_wday) {
    case 0:  day_name = "Sun"; break;
    case 1:  day_name = "Mon"; break;
    case 2:  day_name = "Tue"; break;
    case 3:  day_name = "Wed"; break;
    case 4:  day_name = "Thu"; break;
    case 5:  day_name = "Fri"; break;
    case 6:  day_name = "Sat"; break;
    default: return "";
  }

  std::string month_name;
  switch(timeptr.tm_mon) {
    case 0:  month_name = "Jan"; break;
    case 1:  month_name = "Feb"; break;
    case 2:  month_name = "Mar"; break;
    case 3:  month_name = "Apr"; break;
    case 4:  month_name = "May"; break;
    case 5:  month_name = "Jun"; break;
    case 6:  month_name = "Jul"; break;
    case 7:  month_name = "Aug"; break;
    case 8:  month_name = "Sep"; break;
    case 9:  month_name = "Oct"; break;
    case 10: month_name = "Nov"; break;
    case 11: month_name = "Dec"; break;
    default: return "";
  }

  const int maxlen = 50;
  char res[maxlen];
  snprintf(res, maxlen, "%s, %02d %s %04d %02d:%02d:%02d GMT",
    day_name.c_str(),
    timeptr.tm_mday,
    month_name.c_str(),
    timeptr.tm_year + 1900,
    timeptr.tm_hour,
    timeptr.tm_min,
    timeptr.tm_sec
  );

  return std::string(res);
}

// Given a date string of format "Wed, 21 Oct 2015 07:28:00 GMT", return a
// time_t representing that time. If the date is malformed, then return 0.
time_t parse_http_date_string(const std::string& date);

SEXP log_level(SEXP level_sxp);

// Compares two strings in constant time. Returns true if they are the same;
// false otherwise.
inline bool constant_time_compare(const std::string& a, const std::string& b) {
  if (a.size() != b.size())
    return false;

  volatile const char* ac = a.c_str();
  volatile const char* bc = b.c_str();
  volatile char result = 0;
  int len = (int)a.size();

  for (int i=0; i<len; i++) {
    result |= ac[i] ^ bc[i];
  }

  return (result == 0);
}

#endif
