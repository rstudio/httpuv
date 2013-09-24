#include "websockets-hybi03.h"

#include <assert.h>
#include <string.h>

extern "C" {
#include "md5.h"
}

bool calculateKeyValue(const std::string& key, uint32_t* pResult = NULL) {
  std::string trimmed = trim(key);
  uint32_t value = 0;
  uint32_t spaces = 0;
  for (std::string::const_iterator it = trimmed.begin();
       it != trimmed.end();
       it++) {
    if (*it == ' ')
      spaces ++;
    else if (*it >= '0' && *it <= '9') {
      value *= 10;
      value += *it - '0';
    }
  }
  if (spaces == 0)
    return false;
  if (pResult)
    *pResult = value / spaces;
  return true;
}

bool WebSocketProto_HyBi03::canHandle(const RequestHeaders& requestHeaders,
                                      const char* pData, size_t len) const {

  if (len != 8)
    return false;
  if (requestHeaders.find("sec-websocket-key1") == requestHeaders.end())
    return false;
  if (requestHeaders.find("sec-websocket-key2") == requestHeaders.end())
    return false;
  if (!calculateKeyValue(requestHeaders.at("sec-websocket-key1")) ||
      !calculateKeyValue(requestHeaders.at("sec-websocket-key2"))) {
    return false;
  }
  if (requestHeaders.find("host") == requestHeaders.end())
    return false;

  return requestHeaders.find("upgrade") != requestHeaders.end() &&
         strcasecmp(requestHeaders.at("upgrade").c_str(), "websocket") == 0;
}

void WebSocketProto_HyBi03::handshake(const std::string& url,
                                      const RequestHeaders& requestHeaders,
                                      char** ppData, size_t* pLen,
                                      ResponseHeaders* pResponseHeaders,
                                      std::vector<uint8_t>* pResponse) const {

  assert(*pLen >= 8);

  uint32_t key1, key2;
  calculateKeyValue(requestHeaders.at("sec-websocket-key1"), &key1);
  calculateKeyValue(requestHeaders.at("sec-websocket-key2"), &key2);

  uint8_t handshake[16];
  *reinterpret_cast<uint32_t*>(handshake) = key1;
  *reinterpret_cast<uint32_t*>(handshake + 4) = key2;
  if (!isBigEndian()) {
    swapByteOrder(handshake, handshake + 4);
    swapByteOrder(handshake + 4, handshake + 8);
  }
  memcpy(handshake + 8, *ppData, 8);
  *ppData += 8;
  *pLen -= 8;

  MD5_CTX ctx;
  MD5_Init(&ctx);

  MD5_Update(&ctx, handshake, 16);

  pResponse->resize(16, 0);
  MD5_Final(&(*pResponse)[0], &ctx);

  std::string origin;
  if (requestHeaders.find("sec-websocket-origin") != requestHeaders.end())
    origin = requestHeaders.at("sec-websocket-origin");
  else if (requestHeaders.find("origin") != requestHeaders.end())
    origin = requestHeaders.at("origin");

  std::string location("ws://");
  location += requestHeaders.at("host");
  location += url;

  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Connection", "Upgrade"));
  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Upgrade", "WebSocket"));
  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Sec-WebSocket-Origin", origin));
  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Sec-WebSocket-Location", location));
}

bool WebSocketProto_HyBi03::isFin(uint8_t firstBit) const {
  return firstBit == 0;
}

uint8_t WebSocketProto_HyBi03::toFin(bool isFin) const {
  return isFin ? 0 : 1;
}

Opcode WebSocketProto_HyBi03::decodeOpcode(uint8_t rawCode) const {
  switch (rawCode) {
  case 0:   return Continuation;
  case 1:   return Close;
  case 2:   return Ping;
  case 3:   return Pong;
  case 4:   return Text;
  case 5:   return Binary;
  default:  return Reserved;
  }
}

uint8_t WebSocketProto_HyBi03::encodeOpcode(Opcode opcode) const {
  switch (opcode) {
  case Continuation: return 0;
  case Close:        return 1;
  case Ping:         return 2;
  case Pong:         return 3;
  case Text:         return 4;
  case Binary:       return 5;
  case Reserved:
  default:           return 6; // not expected
  }
}
