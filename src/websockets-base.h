#ifndef WEBSOCKETS_BASE_H
#define WEBSOCKETS_BASE_H

#include "constants.h"

class WebSocketProto {

public:
  WebSocketProto() {}
  virtual ~WebSocketProto() {}

  virtual bool canHandle(const RequestHeaders& requestHeaders,
                         char* pData, size_t len) = 0;

  virtual void handshake(const RequestHeaders& requestHeaders,
                         char* pData, size_t len,
                         ResponseHeaders* responseHeaders) = 0;

  virtual void createFrameHeader(Opcode opcode, bool mask, size_t payloadSize,
                                 int32_t maskingKey,
                                 char pData[MAX_HEADER_BYTES], size_t* pLen) = 0;
};

bool isBigEndian();
// Swaps the byte range [pStart, pEnd)
void swapByteOrder(unsigned char* pStart, unsigned char* pEnd);

#endif // WEBSOCKETS_BASE_H
