#ifndef WSEPERMESSAGEDEFLATE_H
#define WSEPERMESSAGEDEFLATE_H

#include "constants.h"
#include "websockets-base.h"

namespace permessage_deflate {

bool isValid(const RequestHeaders& requestHeaders);

void handshake(const RequestHeaders& requestHeaders,
  ResponseHeaders* pResponseHeaders,
  WebSocketConnectionContext* pContext);

} // namespace permessage_deflate

#endif