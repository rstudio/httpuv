#include "callback.h"

// Invoke a callback and delete the object. The Callback object must have been
// heap-allocated.
void invoke_callback(void* data) {
  Callback* cb = reinterpret_cast<Callback*>(data);
  (*cb)();
  delete cb;
}

// Schedule a std::function<void(void)> to be invoked with later().
void invoke_later(std::function<void(void)> f, double secs) {
  StdFunctionCallback* b_fun = new StdFunctionCallback(f);
  later::later(invoke_callback, (void*)b_fun, secs);
}
