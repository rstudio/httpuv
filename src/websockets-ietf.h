#ifndef WEBSOCKETS_IETF_H
#define WEBSOCKETS_IETF_H

#include "websockets-base.h"

class WebSocketProto_IETF : WebSocketProto {

public:
  WebSocketProto_IETF() {}
  virtual ~WebSocketProto_IETF() {}

  bool canHandle(const RequestHeaders& requestHeaders,
                 char* pData, size_t len);

  void handshake(const RequestHeaders& requestHeaders,
                 char* pData, size_t len,
                 ResponseHeaders* responseHeaders);

  void createFrameHeader(Opcode opcode, bool mask, size_t payloadSize,
                         int32_t maskingKey,
                         char pData[MAX_HEADER_BYTES], size_t* pLen);

};

#endif // WEBSOCKETS_IETF_H
