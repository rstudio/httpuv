#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <map>

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

template<class T>
class ScopePtr {
  T* _p;
public:
  explicit ScopePtr(T* p) : _p(p) {
  }
  ~ScopePtr() {
    delete _p;
  }
  T & operator*() const {
    return *_p;
  }

  T * operator->() const {
    return _p;
  }
};

#endif // CONSTANTS_H
