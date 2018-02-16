#include "callback.h"

// Invoke a callback and delete the object. The Callback object must have been
// heap-allocated.
void invoke_callback(void* data) {
  Callback* cb = reinterpret_cast<Callback*>(data);
  (*cb)();
  delete cb;
}

// Schedule a boost::function<void(void)> to be invoked with later().
void invoke_later(boost::function<void(void)> f, double secs) {
  BoostFunctionCallback* b_fun = new BoostFunctionCallback(f);
  later::later(invoke_callback, (void*)b_fun, secs);
}
