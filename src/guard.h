#ifndef GUARD_HPP
#define GUARD_HPP

#include <uv.h>

class guard {
public:
  guard(uv_mutex_t &mutex) {
    _mutex = &mutex;
    uv_mutex_lock(_mutex);
  }

  ~guard() {
    uv_mutex_unlock(_mutex);
  }

private:
  uv_mutex_t* _mutex;
};

#endif
