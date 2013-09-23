#ifndef WEBSOCKETS_HYBI03_H
#define WEBSOCKETS_HYBI03_H

#include "websockets-base.h"

class WebSocketProto_HyBi03 : public WebSocketProto {

public:
  WebSocketProto_HyBi03() {}
  virtual ~WebSocketProto_HyBi03() {}

  bool canHandle(const RequestHeaders& requestHeaders,
                 const char* pData, size_t len) const;

  void handshake(const RequestHeaders& requestHeaders,
                 const char* pData, size_t len,
                 ResponseHeaders* pResponseHeaders,
                 std::vector<uint8_t>* pResponse) const;

  void createFrameHeader(Opcode opcode, bool mask, size_t payloadSize,
                         int32_t maskingKey,
                         char pData[MAX_HEADER_BYTES], size_t* pLen) const;

  bool isFin(uint8_t firstBit) const;
  uint8_t toFin(bool isFin) const;
  Opcode decodeOpcode(uint8_t rawCode) const;
  uint8_t encodeOpcode(Opcode opcode) const;
};

#endif // WEBSOCKETS_HYBI03_H
