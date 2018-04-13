#include "callbackqueue.h"
#include "tqueue.h"
#include "thread.h"
#include <boost/function.hpp>
#include "libuv/include/uv.h"


// This non-class function is a plain C wrapper for CallbackQueue::flush(), and
// is needed as a callback to pass to uv_async_send.
void flush_callback_queue(uv_async_t *handle) {
  CallbackQueue* wq = reinterpret_cast<CallbackQueue*>(handle->data);
  wq->flush();
}


CallbackQueue::CallbackQueue(uv_loop_t* loop) {
  ASSERT_BACKGROUND_THREAD()
  uv_async_init(loop, &flush_handle, flush_callback_queue);
  flush_handle.data = reinterpret_cast<void*>(this);
}


void CallbackQueue::push(boost::function<void (void)> cb) {
  q.push(cb);
  uv_async_send(&flush_handle);
}

void CallbackQueue::flush() {
  ASSERT_BACKGROUND_THREAD()
  boost::function<void (void)> cb;

  while (1) {
    // Do queue operations inside this guarded scope, but we'll execute the
    // callback outside of the scope, since it doesn't need to be protected,
    // and this will make it possible for the other thread to do queue
    // operations while we're invoking the callback.
    {
      guard guard(q.mutex);
      if (q.size() == 0) {
        break;
      }

      cb = q.front();
      q.pop();
    }

    cb();
  }
}
