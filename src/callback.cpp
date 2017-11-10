#include "callback.h"

// Invoke a callback and delete the object. The Callback object must have been
// heap-allocated.
void invoke_callback(void* data) {
  Callback* cb = reinterpret_cast<Callback*>(data);
  (*cb)();
  delete cb;
}
