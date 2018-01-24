#ifndef THREAD_HPP
#define THREAD_HPP

#include <uv.h>

// These must be called from the main and background thread, respectively, so
// that is_main_thread() and is_background_thread() can be tested later.
void register_main_thread();
void register_background_thread();

bool is_main_thread();
bool is_background_thread();


#ifdef DEBUG_THREAD
#include <assert.h>

// This must be called from the main thread so that thread assertions can be
// tested later.
#define ASSERT_MAIN_THREAD()         assert(is_main_thread());
#define ASSERT_BACKGROUND_THREAD()   assert(is_background_thread());

#else
#define ASSERT_MAIN_THREAD()
#define ASSERT_BACKGROUND_THREAD()

#endif // DEBUG_THREAD


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


// Container class for holding thread-safe values.
template <typename T>
class ThreadSafe {
public:
  ThreadSafe(const T value) : _value(value) {
    uv_mutex_init(&_mutex);
  };

  void set(const T value) {
    guard guard(_mutex);
    _value = value;
  };

  T get() {
    guard guard(_mutex);
    return _value;
  };

private:
  T _value;
  uv_mutex_t _mutex;
};



// Wrapper class for mutex/condition variable pair.
class CondWait {
public:
  CondWait() {
    uv_mutex_init(&mutex);
    uv_cond_init(&cond);
  };

  ~CondWait() {
    uv_mutex_destroy(&mutex);
    uv_cond_destroy(&cond);
  }

  void lock() {
    uv_mutex_lock(&mutex);
  }

  void signal() {
    uv_cond_signal(&cond);
    uv_mutex_unlock(&mutex);
  };

  void wait() {
    uv_mutex_lock(&mutex);
    uv_cond_wait(&cond, &mutex);
    uv_mutex_unlock(&mutex);
  }

  uv_mutex_t mutex;
  uv_cond_t cond;
};

#endif
