#ifndef AUTO_DELETER_HPP
#define AUTO_DELETER_HPP

#include <boost/bind.hpp>
#include "callbackqueue.h"
#include "thread.h"
#include <later_api.h>


extern CallbackQueue* background_queue;


// A deleter function, which, if called on the main thread, will delete the
// object immediately. If called on the background thread, it will schedule
// deletion to happen on the main thread. This is useful in cases where we
// don't know ahead of time which thread will be triggering the deletion.
template <typename T, bool trycatch = false>
void auto_deleter_main(T* obj) {
  if (is_main_thread()) {
    deleter_main<T, trycatch>(obj);

  } else if (is_background_thread()) {
    later::later(deleter_main<T, trycatch>, obj, 0);

  } else {
    throw std::runtime_error("Can't detect correct thread for auto_deleter_main.");
  }
}

// A deleter function, which, if called on the background thread, will delete
// the object immediately. If called on the main thread, it will schedule
// deletion to happen on the background thread. This is useful in cases where
// we don't know ahead of time which thread will be triggering the deletion.
template <typename T, bool trycatch = false>
void auto_deleter_background(T* obj) {
  if (is_main_thread()) {
    background_queue->push(boost::bind(deleter_background<T, trycatch>, obj));

  } else if (is_background_thread()) {
    deleter_background<T, trycatch>(obj);

  } else {
    throw std::runtime_error("Can't detect correct thread for auto_deleter_background.");
  }
}


#endif
