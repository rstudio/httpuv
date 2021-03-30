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

// @param input The istream to parse from
// @param digits The exact number of digits to parse; if this number of digits
//   is not available, false is returned.
// @param pOut If true is returned, the integer value of the parsed value. If
//   false returned, pOut is untouched.
// @return true if successful, false if parsing fails for any reason
bool str_read_int(std::istream* input, size_t digits, int* pOut) {
  if (digits <= 0) {
    return false;
  }
  int tmp = 0;
  while (digits-- > 0) {
    if (input->fail() || input->eof()) {
      return false;
    }
    int b = input->get();
    if (b == EOF) {
      return false;
    }
    char c = (char)b;
    if (c < '0' || c > '9') {
      return false;
    }
    int v = c - '0';
    *pOut = (*pOut * 10) + v;
  }
  return true;
}

// @param input The istream to parse from
// @param bytes The exact number of bytes to read from the input. If this many
//   bytes are not available, false is returned.
// @param Array where each element is a char* of length `bytes`; the final
//   element must be NULL, or else the function will probably segfault.
// @param pRes If true is returned, then this will be set to the index of the
//   element in `values` that matched the input. If false is returned, then res
//   will be untouched.
// @return true if successful, false if reading failed or no match found
bool str_read_lookup(std::istream* input, size_t bytes, const char** values, int* pRes) {
  std::vector<char> buf;
  buf.resize(bytes + 1);

  input->get(&buf[0], bytes + 1, '\0');
  int i = 0;
  if (input->fail() || input->eof()) {
    return false;
  }

  while (true) {
    const char* option = values[i];
    if (option == NULL) {
      // Got to end of list and no match
      return false;
    }
    if (!strncmp(&buf[0], option, bytes)) {
      break;
    }
    i++;
  }

  *pRes = i;
  return true;
}

const char* MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL};
const char* DAYS_OF_WEEK[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL};

// Given a date string of format "Wed, 21 Oct 2015 07:28:00 GMT", return a
// time_t representing that time. If the date is malformed, then return 0.
time_t parse_http_date_string(const std::string& date) {
  // This is because the static std::locale may not be thread-safe. If in the
  // future we need to call this from multiple threads, we can remove this and
  // make the std::locale non-static.
  ASSERT_BACKGROUND_THREAD()

  if (date.length() != 29) {
    return 0;
  }

  std::tm t = {0};

  try {
    std::istringstream date_ss(date);

    // // This is a much nicer way of parsing the time, but std::get_time is not
    // // available on the libstdc++ that ships with Rtools35.exe. Until we can
    // // drop support for R 3.x, we're stuck with manual parsing.
    // std::locale c_locale("C");
    // date_ss.imbue(c_locale);
    // date_ss >> std::get_time(&t, "%a, %d %b %Y %H:%M:%S GMT");
    // if (date_ss.fail()) {
    //   return 0;
    // }

    if (!str_read_lookup(&date_ss, 3, DAYS_OF_WEEK, &t.tm_wday)) return 0;
    if (date_ss.get() != ',') return 0;
    if (date_ss.get() != ' ') return 0;
    if (!str_read_int(&date_ss, 2, &t.tm_mday)) return 0;
    if (date_ss.get() != ' ') return 0;
    if (!str_read_lookup(&date_ss, 3, MONTHS, &t.tm_mon)) return 0;
    if (date_ss.get() != ' ') return 0;
    int year = 0;
    if (!str_read_int(&date_ss, 4, &year)) return 0;
    t.tm_year = year - 1900;
    if (date_ss.get() != ' ') return 0;
    if (!str_read_int(&date_ss, 2, &t.tm_hour)) return 0;
    if (date_ss.get() != ':') return 0;
    if (!str_read_int(&date_ss, 2, &t.tm_min)) return 0;
    if (date_ss.get() != ':') return 0;
    if (!str_read_int(&date_ss, 2, &t.tm_sec)) return 0;
    if (date_ss.get() != ' ') return 0;
    if (date_ss.get() != 'G') return 0;
    if (date_ss.get() != 'M') return 0;
    if (date_ss.get() != 'T') return 0;
    if (date_ss.get() != EOF) return 0;
  } catch(...) {
    return 0;
  }

  return timegm(&t);
}
