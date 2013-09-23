#ifndef WEBSOCKETS_BASE_H
#define WEBSOCKETS_BASE_H

#include <stdint.h>

#include "constants.h"

class WebSocketProto {

public:
  WebSocketProto() {}
  virtual ~WebSocketProto() {}

  // Return true if the request uses this protocol version and is valid
  virtual bool canHandle(const RequestHeaders& requestHeaders,
                         const char* pData, size_t len) const = 0;

  // Populate response headers with the appropriate values. This call
  // must not fail, but it will not be called unless canHandle returned
  // true previously, so any validation should be done in canHandle.
  virtual void handshake(const RequestHeaders& requestHeaders,
                         const char* pData, size_t len,
                         ResponseHeaders* responseHeaders,
                         std::vector<uint8_t>* pResponse) const = 0;

  void createFrameHeader(Opcode opcode, bool mask, size_t payloadSize,
                         int32_t maskingKey,
                         char pData[MAX_HEADER_BYTES], size_t* pLen) const;

  virtual bool isFin(uint8_t firstBit) const = 0;
  virtual uint8_t toFin(bool isFin) const = 0;
  virtual Opcode decodeOpcode(uint8_t rawCode) const = 0;
  virtual uint8_t encodeOpcode(Opcode opcode) const = 0;
};

bool isBigEndian();
// Swaps the byte range [pStart, pEnd)
void swapByteOrder(unsigned char* pStart, unsigned char* pEnd);

#endif // WEBSOCKETS_BASE_H
