#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "optional.h"
#include "thread.h"
#include "timegm.h"

#include "cpp11.hpp"

using namespace cpp11;

// A callback for deleting objects on the main thread using later(). This is
// needed when the object is an Rcpp object or contains one, because deleting
// such objects invoke R's memory management functions.
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

// This is used for converting a named R vector (T2) to a std::map.
template <typename T1, typename T2>
std::map<std::string, T1> toMap(T2 x) {
  ASSERT_MAIN_THREAD()

  std::map<std::string, T1> strmap;

  if (x.size() == 0) {
    return strmap;
  }

  strings names = x.names();
  if (Rf_isNull(SEXP(names))) {
    stop("Error converting R object to map<string, T>: vector does not have names.");
  }

  for (R_xlen_t i = 0; i < x.size(); i++) {
    std::string name  = std::string(names[i]);
    T1          value = as_cpp<T1>(SEXP(x[i]));
    if (name == "") {
      stop("Error converting R object to map<string, T>: element has empty name.");
    }

    strmap.insert(
      std::pair<std::string, T1>(name, value)
    );
  }

  return strmap;
}

// A wrapper for as_cpp. If the R value is NULL, this returns nullopt;
// otherwise it returns the usual value that as_cpp returns, wrapped in
// std::experimental::optional<T1>.
template <typename T1>
std::experimental::optional<T1> optional_as(SEXP value) {
  if (Rf_isNull(value)) {
    return std::experimental::nullopt;
  }
  return std::experimental::optional<T1>(as_cpp<T1>(value));
}

// If the C++ value is missing, this returns R NULL; otherwise converts to SEXP.
template <typename T>
SEXP optional_wrap(std::experimental::optional<T> value) {
  if (!value.has_value()) {
    return R_NilValue;
  }
  return as_sexp(*value);
}


// as_cpp and as_sexp for ResponseHeaders. Since the ResponseHeaders typedef is
// in constants.h and this file doesn't include constants.h, we define them
// using the actual vector type instead of the ResponseHeaders typedef.
inline std::vector<std::pair<std::string, std::string>> as_response_headers(SEXP x) {
  ASSERT_MAIN_THREAD()
  strings hdrs(x);
  strings nms = hdrs.names();

  if (Rf_isNull(SEXP(nms))) {
    stop("All values must be named.");
  }

  std::vector<std::pair<std::string, std::string>> result;
  for (R_xlen_t i = 0; i < hdrs.size(); i++) {
    std::string name = std::string(nms[i]);
    if (name.empty()) {
      stop("All values must be named.");
    }
    result.push_back({name, std::string(hdrs[i])});
  }
  return result;
}

inline SEXP response_headers_to_sexp(const std::vector<std::pair<std::string, std::string>>& x) {
  ASSERT_MAIN_THREAD()
  R_xlen_t n = static_cast<R_xlen_t>(x.size());
  writable::strings values(n);
  writable::strings nms(n);
  for (R_xlen_t i = 0; i < n; i++) {
    nms[i]    = x[i].first;
    values[i] = x[i].second;
  }
  values.attr("names") = nms;
  return values;
}

// optional_as / optional_wrap specializations for ResponseHeaders.
template <>
inline std::experimental::optional<std::vector<std::pair<std::string, std::string>>>
optional_as<std::vector<std::pair<std::string, std::string>>>(SEXP value) {
  if (Rf_isNull(value)) {
    return std::experimental::nullopt;
  }
  return as_response_headers(value);
}

template <>
inline SEXP optional_wrap(std::experimental::optional<std::vector<std::pair<std::string, std::string>>> value) {
  if (!value.has_value()) {
    return R_NilValue;
  }
  return response_headers_to_sexp(*value);
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

// Compares two strings in constant time. Returns true if they are the same;
// false otherwise.
inline bool constant_time_compare(const std::string& a, const std::string& b) {
  if (a.length() != b.length())
    return false;

  volatile const char* ac = a.c_str();
  volatile const char* bc = b.c_str();
  volatile char result = 0;
  int len = a.length();

  for (int i=0; i<len; i++) {
    result |= ac[i] ^ bc[i];
  }

  return (result == 0);
}

#endif
