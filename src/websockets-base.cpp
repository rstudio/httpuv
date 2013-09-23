#include "websockets-base.h"

bool isBigEndian() {
  uint32_t i = 1;
  return *((uint8_t*)&i) == 0;
}

// Swaps the byte range [pStart, pEnd)
void swapByteOrder(unsigned char* pStart, unsigned char* pEnd) {
  // Easier for callers to use exclusive end but easier to implement
  // using inclusive end
  pEnd--;

  while (pStart < pEnd) {

    unsigned char tmp;
    tmp = *pStart;
    *pStart = *pEnd;
    *pEnd = tmp;

    pStart++;
    pEnd--;
  }
}
