#ifndef WEBSOCKETS_IETF_H
#define WEBSOCKETS_IETF_H

#include "websockets-base.h"

class WebSocketProto_IETF : public WebSocketProto {

public:
  WebSocketProto_IETF() {}
  virtual ~WebSocketProto_IETF() {}

  bool canHandle(const RequestHeaders& requestHeaders,
                 const char* pData, size_t len) const;

  void handshake(const std::string& url,
                 const RequestHeaders& requestHeaders,
                 char** ppData, size_t* pLen,
                 ResponseHeaders* responseHeaders,
                 std::vector<uint8_t>* pResponse) const;

  bool isFin(uint8_t firstBit) const;
  uint8_t toFin(bool isFin) const;
  Opcode decodeOpcode(uint8_t rawCode) const;
  uint8_t encodeOpcode(Opcode opcode) const;
};

#endif // WEBSOCKETS_IETF_H
