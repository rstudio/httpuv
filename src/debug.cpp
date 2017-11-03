#include "debug.h"
#include <string>
#include <iostream>

#ifdef DEBUG_THREAD
uv_thread_t __main_thread__;
#endif


void trace(const std::string& msg) {
#ifdef DEBUG_TRACE
  std::cerr << msg << std::endl;
#endif
}
