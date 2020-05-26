#include <cstring>
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

void WebSocketProto::createFrameHeader(
    Opcode opcode, bool mask, size_t payloadSize, int32_t maskingKey,
    char pData[MAX_HEADER_BYTES], size_t* pLen) const {

  unsigned char* pBuf = (unsigned char*)pData;
  unsigned char* pMaskingKey = pBuf + 2;
  // Need to copy from a 64-bit chunk of memory, but size_t may be smaller.
  uint64_t payloadSize_64 = payloadSize;

  pBuf[0] =
    toFin(true) << 7 | // FIN; always true
    encodeOpcode(opcode);
  pBuf[1] = mask ? 1 << 7 : 0;
  if (payloadSize_64 <= 125) {
    pBuf[1] |= payloadSize_64;
    pMaskingKey = pBuf + 2;
  }
  else if (payloadSize_64 <= 65535) {// 2^16-1
    pBuf[1] |= 126;
    memcpy(pBuf + 2, &payloadSize_64, sizeof(uint16_t));
    if (!isBigEndian())
      swapByteOrder(pBuf + 2, pBuf + 4);
    pMaskingKey = pBuf + 4;
  }
  else {
    pBuf[1] |= 127;
    memcpy(pBuf + 2, &payloadSize_64, sizeof(uint64_t));
    if (!isBigEndian())
      swapByteOrder(pBuf + 2, pBuf + 10);
    pMaskingKey = pBuf + 10;
  }

  if (mask) {
    memcpy(pMaskingKey, &maskingKey, sizeof(int32_t));
  }

  *pLen = (pMaskingKey - pBuf) + (mask ? 4 : 0);
}
