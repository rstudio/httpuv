#include "websockets-ietf.h"

#include <sha1.h>
#include <base64.hpp>

bool WebSocketProto_IETF::canHandle(const RequestHeaders& requestHeaders,
                                    char* pData, size_t len) {

  return requestHeaders.find("upgrade") != requestHeaders.end() &&
         strcasecmp(requestHeaders.at("upgrade").c_str(), "websocket") == 0 &&
         requestHeaders.find("sec-websocket-key") != requestHeaders.end();
}

void WebSocketProto_IETF::handshake(const RequestHeaders& requestHeaders,
                                    char* pData, size_t len,
                                    ResponseHeaders* pResponseHeaders) {

  std::string key = requestHeaders.at("sec-websocket-key");

  std::string clear = trim(key) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA1_CTX ctx;
  reid_SHA1_Init(&ctx);
  reid_SHA1_Update(&ctx, (uint8_t*)&clear[0], clear.size());

  std::vector<uint8_t> digest(SHA1_DIGEST_SIZE);
  reid_SHA1_Final(&ctx, &digest[0]);

  std::string response = b64encode(digest);

  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Connection", "Upgrade"));
  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Upgrade", "websocket"));
  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Sec-WebSocket-Accept", response));
}

void WebSocketProto_IETF::createFrameHeader(
    Opcode opcode, bool mask, size_t payloadSize, int32_t maskingKey,
    char pData[MAX_HEADER_BYTES], size_t* pLen) {

  unsigned char* pBuf = (unsigned char*)pData;
  unsigned char* pMaskingKey = pBuf + 2;

  pBuf[0] =
    1 << 7 | // FIN; always true
    opcode;
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
