#ifndef CALLBACKQUEUE_HPP
#define CALLBACKQUEUE_HPP

#include "queue.h"
#include <boost/function.hpp>
#include "libuv/include/uv.h"

class CallbackQueue {
public:
  CallbackQueue(uv_loop_t* loop);
  void push(boost::function<void (void)> cb);
  // Needs to be a friend to call .flush()
  friend void flush_callback_queue(uv_async_t *handle);

private:
  void flush();
  uv_async_t flush_handle;
  queue< boost::function<void (void)> > q;
};


#endif
