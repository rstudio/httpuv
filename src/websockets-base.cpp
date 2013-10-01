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

  pBuf[0] =
    toFin(true) << 7 | // FIN; always true
    encodeOpcode(opcode);
  pBuf[1] = mask ? 1 << 7 : 0;
  if (payloadSize <= 125) {
    pBuf[1] |= payloadSize;
    pMaskingKey = pBuf + 2;
  }
  else if (payloadSize <= 65535) {// 2^16-1
    pBuf[1] |= 126;
    *((uint16_t*)&pBuf[2]) = (uint16_t)payloadSize;
    if (!isBigEndian())
      swapByteOrder(pBuf + 2, pBuf + 4);
    pMaskingKey = pBuf + 4;
  }
  else {
    pBuf[1] |= 127;
    *((uint64_t*)&pBuf[2]) = (uint64_t)payloadSize;
    if (!isBigEndian())
      swapByteOrder(pBuf + 2, pBuf + 10);
    pMaskingKey = pBuf + 10;
  }

  if (mask) {
    *((int32_t*)pMaskingKey) = maskingKey;
  }

  *pLen = (pMaskingKey - pBuf) + (mask ? 4 : 0);
}
