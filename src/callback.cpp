#include "callback.h"

void invoke_callback(void* data) {
  Callback* cb = reinterpret_cast<Callback*>(data);
  (*cb)();
  delete cb;
}
