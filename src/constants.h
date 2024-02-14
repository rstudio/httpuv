#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <strings.h>

#include <string>
#include <map>
#include <vector>

enum Opcode {
  Continuation = 0,
  Text = 1,
  Binary = 2,
  Close = 8,
  Ping = 9,
  Pong = 0xA,
  Reserved = 0xF
};

// Maximum possible byte length for WebSocket header
const size_t MAX_HEADER_BYTES = 14;
const size_t MAX_FOOTER_BYTES = 1;

enum WSParseState {
  InHeader,
  InPayload
};

struct compare_ci {
  bool operator()(const std::string& a, const std::string& b) const {
    return strcasecmp(a.c_str(), b.c_str()) < 0;
  }
};

typedef std::map<std::string, std::string, compare_ci> RequestHeaders;

typedef std::vector<std::pair<std::string, std::string> > ResponseHeaders;

class NoCopy {
protected:
  NoCopy() {}
  ~NoCopy() {}
private:
  NoCopy(const NoCopy&) {}
  const NoCopy& operator=(const NoCopy& a) { return a; }
};

// trim from both ends
static inline std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of("\t ");
  if (start == std::string::npos)
    return std::string();
  size_t end = s.find_last_not_of("\t ") + 1;
  return s.substr(start, end-start);
}

static inline std::vector<std::string> split(const std::string& s, const std::string& delim) {
  std::vector<std::string> results;
  size_t pos = 0;
  while (true) {
    size_t i = s.find(delim, pos);
    if (i == std::string::npos) {
      break;
    }
    if (i != pos) {
      results.push_back(s.substr(pos, i - pos));
    }
    pos = i + 1;
  }
  if (pos != s.length()) {
    results.push_back(s.substr(pos));
  }
  return results;
}

#endif // CONSTANTS_H
