#ifndef WRITEQUEUE_HPP
#define WRITEQUEUE_HPP

#include "queue.h"
#include <boost/function.hpp>
#include <uv.h>

class WriteQueue {
public:
  WriteQueue(uv_loop_t* loop);
  void push(boost::function<void (void)> cb);
  void flush();

private:
  uv_async_t flush_handle;
  queue< boost::function<void (void)> > q;
};


#endif
