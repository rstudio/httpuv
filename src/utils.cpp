#include "utils.h"

// Set the default log level
LogLevel log_level_ = ERROR;

void debug_log(const std::string& msg, LogLevel level) {
  if (log_level_ >= level) {
    err_printf("%s\n", msg.c_str());
  };
}


// Sets the current log level and returns previous value.
// [[Rcpp::export]]
std::string log_level(std::string level) {
  LogLevel old_level = log_level_;

  if (level == "") {
    // Do nothing
  } else if (level == "OFF") {
    log_level_ = OFF;
  } else if (level == "ERROR") {
    log_level_ = ERROR;
  } else if (level == "WARN") {
    log_level_ = WARN;
  } else if (level == "INFO") {
    log_level_ = INFO;
  } else if (level == "DEBUG") {
    log_level_ = DEBUG;
  } else {
    Rcpp::stop("Unknown value for `level`");
  }

  switch(old_level) {
    case OFF:   return "OFF";
    case ERROR: return "ERROR";
    case WARN:  return "WARN";
    case INFO:  return "INFO";
    case DEBUG: return "DEBUG";
  }
}
