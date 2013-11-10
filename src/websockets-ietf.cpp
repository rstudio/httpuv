#include "websockets-ietf.h"

#include <sha1.h>
#include <base64.hpp>

bool WebSocketProto_IETF::canHandle(const RequestHeaders& requestHeaders,
                                    const char* pData, size_t len) const {

  return requestHeaders.find("upgrade") != requestHeaders.end() &&
         strcasecmp(requestHeaders.at("upgrade").c_str(), "websocket") == 0 &&
         requestHeaders.find("sec-websocket-key") != requestHeaders.end();
}

void WebSocketProto_IETF::handshake(const std::string& url,
                                    const RequestHeaders& requestHeaders,
                                    char** ppData, size_t* pLen,
                                    ResponseHeaders* pResponseHeaders,
                                    std::vector<uint8_t>* pResponse) const {

  std::string key = requestHeaders.at("sec-websocket-key");

  std::string clear = trim(key) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA1_CTX ctx;
  reid_SHA1_Init(&ctx);
  reid_SHA1_Update(&ctx, (uint8_t*)&clear[0], clear.size());

  std::vector<uint8_t> digest(SHA1_DIGEST_SIZE);
  reid_SHA1_Final(&ctx, &digest[0]);

  std::string response = b64encode(digest.begin(), digest.end());

  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Connection", "Upgrade"));
  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Upgrade", "websocket"));
  pResponseHeaders->push_back(
    std::pair<std::string, std::string>("Sec-WebSocket-Accept", response));
}

bool WebSocketProto_IETF::isFin(uint8_t firstBit) const {
  return firstBit != 0;
}

uint8_t WebSocketProto_IETF::toFin(bool isFin) const {
  return isFin ? 1 : 0;
}

Opcode WebSocketProto_IETF::decodeOpcode(uint8_t rawCode) const {
  switch (rawCode) {
  case 0:   return Continuation;
  case 1:   return Text;
  case 2:   return Binary;
  case 8:   return Close;
  case 9:   return Ping;
  case 0xA: return Pong;
  case 0xF: return Reserved;
  default:  return Reserved;
  }
}

uint8_t WebSocketProto_IETF::encodeOpcode(Opcode opcode) const {
  switch (opcode) {
  case Continuation:   return 0;
  case Text:           return 1;
  case Binary:         return 2;
  case Close:          return 8;
  case Ping:           return 9;
  case Pong:           return 0xA;
  case Reserved:       return 0xF;
  default:             return 0xF; // not expected
  }
}
