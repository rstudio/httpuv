#include "utils.h"

// Set the default log level
LogLevel log_level_ = LOG_ERROR;

void debug_log(const std::string& msg, LogLevel level) {
  if (log_level_ >= level) {
    err_printf("%s\n", msg.c_str());
  };
}


// Sets the current log level and returns previous value.
// [[Rcpp::export]]
std::string log_level(const std::string& level) {
  LogLevel old_level = log_level_;

  if (level == "") {
    // Do nothing
  } else if (level == "OFF") {
    log_level_ = LOG_OFF;
  } else if (level == "ERROR") {
    log_level_ = LOG_ERROR;
  } else if (level == "WARN") {
    log_level_ = LOG_WARN;
  } else if (level == "INFO") {
    log_level_ = LOG_INFO;
  } else if (level == "DEBUG") {
    log_level_ = LOG_DEBUG;
  } else {
    Rcpp::stop("Unknown value for `level`");
  }

  switch(old_level) {
    case LOG_OFF:   return "OFF";
    case LOG_ERROR: return "ERROR";
    case LOG_WARN:  return "WARN";
    case LOG_INFO:  return "INFO";
    case LOG_DEBUG: return "DEBUG";
    default:        return "";
  }
}
