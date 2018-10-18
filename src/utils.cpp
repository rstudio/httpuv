#include "utils.h"
#include <iostream>

void trace(const std::string& msg) {
#ifdef DEBUG_TRACE
  std::cerr << msg << std::endl;
#endif
}
