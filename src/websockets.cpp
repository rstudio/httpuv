#include "websockets.h"
#include <assert.h>

#include <algorithm>
#include <iostream>
#include <iomanip>

#include <sha1.h>
#include <base64.hpp>

template <typename T>
T min(T a, T b) {
  return (a > b) ? b : a;
}

std::string dumpbin(const char* data, size_t len) {
  std::string output;
  for (size_t i = 0; i < len; i++) {
    char byte = data[i];
    for (size_t mask = 0x80; mask > 0; mask >>= 1) {
      output.push_back(byte & mask ? '1' : '0');
    }
    if (i % 4 == 3)
      output.push_back('\n');
    else
      output.push_back(' ');
  }
  return output;
}

bool WSFrameHeader::isHeaderComplete() const {
  if (_data.size() < 2)
    return false;

  return _data.size() >= (size_t)headerLength();
}

bool WSFrameHeader::fin() const {
  return read(0, 1) != 0;
}
Opcode WSFrameHeader::opcode() const {
  uint8_t oc = read(4, 4);
  switch (oc) {
  case Continuation:
  case Text:
  case Binary:
  case Close:
  case Ping:
  case Pong:
    return (Opcode)oc;
  default:
    return Reserved;
  }
}
bool WSFrameHeader::masked() const {
  return read(8, 1) != 0;
}
uint64_t WSFrameHeader::payloadLength() const {
  uint8_t pl = read(9, 7);
  switch (pl) {
    case 126:
      return read64(16, 16);
    case 127:
      return read64(16, 64);
    default:
      return pl;
  }
}
void WSFrameHeader::maskingKey(uint8_t key[4]) const {
  if (!masked())
    memset(key, 0, 4);
  else {
    key[0] = read(9 + payloadLengthLength(), 8);
    key[1] = read(9 + payloadLengthLength() + 8, 8);
    key[2] = read(9 + payloadLengthLength() + 16, 8);
    key[3] = read(9 + payloadLengthLength() + 24, 8);
  }
}
size_t WSFrameHeader::headerLength() const {
  return (9 + payloadLengthLength() + maskingKeyLength()) / 8;
}
uint8_t WSFrameHeader::read(size_t bitOffset, size_t bitWidth) const {
  size_t byteOffset = bitOffset / 8;
  bitOffset = bitOffset % 8;

  assert((bitOffset + bitWidth) <= 8);
  assert(byteOffset < _data.size());

  uint8_t mask = 0xFF;
  mask <<= (8 - bitWidth);
  mask >>= bitOffset;

  char byte = _data[byteOffset];
  return (byte & mask) >> (8 - bitWidth - bitOffset);
}
uint64_t WSFrameHeader::read64(size_t bitOffset, size_t bitWidth) const {
  assert((bitOffset % 8) == 0);
  assert((bitWidth % 8) == 0);

  size_t byteOffset = bitOffset / 8;
  size_t byteWidth = bitWidth / 8;
  assert(byteOffset + byteWidth <= _data.size());

  uint64_t result = 0;

  for (size_t i = 0; i < byteWidth; i++) {
    result <<= 8;
    result += (uint64_t)(unsigned char)_data[byteOffset + i];
  }
  
  return result;
}
uint8_t WSFrameHeader::payloadLengthLength() const {
  uint8_t pll = read(9, 7);
  switch (pll) {
    case 126:
      return 7 + 16;
    case 127:
      return 7 + 64;
    default:
      return 7;
  }
}
uint8_t WSFrameHeader::maskingKeyLength() const {
  return masked() ? 32 : 0;
}

void WebSocketParser::read(const char* data, size_t len) {
  while (len > 0) {
    // crude check for underflow
    assert(len < 1000000000000000000);

    switch (_state) {
      case InHeader: {
        // The _header vector<char> accumulates header data until
        // the complete header is read. It's possible/likely it also
        // holds part of the payload.
        size_t startingSize = _header.size();
        std::copy(data, data + min(len, MAX_HEADER_BYTES),
          std::back_inserter(_header));

        WSFrameHeader frame(&_header[0], _header.size());

        if (frame.isHeaderComplete()) {
          onHeaderComplete(frame);

          size_t payloadOffset = frame.headerLength() - startingSize;
          _bytesLeft = frame.payloadLength();

          _state = InPayload;
          _header.clear();

          data += payloadOffset - startingSize;
          len -= payloadOffset - startingSize;
        }
        else {
          // All of the data was consumed, but no header
          data += len;
          len = 0;
        }
        break;
      }
      case InPayload: {
        size_t bytesToConsume = min((uint64_t)len, _bytesLeft);
        _bytesLeft -= bytesToConsume;
        onPayload(data, bytesToConsume);

        data += bytesToConsume;
        len -= bytesToConsume;

        if (_bytesLeft == 0) {
          onFrameComplete();
          
          _state = InHeader;
        }
        break;
      }
      default:
        assert(false);
        break;
    }
  }
}

void WebSocketConnection::sendWSMessage(Opcode opcode, const char* pData, size_t length) {
  std::vector<char> header(MAX_HEADER_BYTES);

  size_t headerLength;
  createFrameHeader(opcode, false, length, 0,
    &header[0], &headerLength);
  header.resize(headerLength);

  sendWSFrame(&header[0], header.size(), pData, length);
}

void WebSocketConnection::closeWS() {
  // If we have already sent a close message, do nothing. It's especially
  // important that we don't call closeWSSocket twice, this might lead to
  // a crash as we (eventually) might double-free the Socket object.
  if (_connState & WS_CLOSE_SENT)
    return;

  // Send the close message
  _connState |= WS_CLOSE_SENT;
  sendWSMessage(Close, NULL, 0);

  // If close messages have been both sent and received, close socket.
  if (_connState == WS_CLOSE)
    closeWSSocket();
}

void WebSocketConnection::onHeaderComplete(const WSFrameHeader& header) {
  _header = header;
  if (!header.fin() && header.opcode() != Continuation)
    _incompleteContentHeader = header;
}
void WebSocketConnection::onPayload(const char* data, size_t len) {
  size_t origSize = _payload.size();
  std::copy(data, data + len, std::back_inserter(_payload));

  if (_header.maskingKeyLength() != 0) {
    uint8_t mask[4];
    _header.maskingKey(mask);
    for (size_t i = origSize; i < _payload.size(); i++) {
      size_t j = i % 4;
      _payload[i] = _payload[i] ^ mask[j];
    }
  }
}
void WebSocketConnection::onFrameComplete() {
  if (!_header.fin()) {
    std::copy(_payload.begin(), _payload.end(),
      std::back_inserter(_incompleteContentPayload));
  } else {
    switch (_header.opcode()) {
      case Continuation: {
        std::copy(_payload.begin(), _payload.end(),
          std::back_inserter(_incompleteContentPayload));
        onWSMessage(_incompleteContentHeader.opcode() == Binary,
          &_incompleteContentPayload[0], _incompleteContentPayload.size());

        _incompleteContentPayload.clear();
        break;
      }
      case Text:
      case Binary: {
        onWSMessage(_header.opcode() == Binary, &_payload[0], _payload.size());
        break;
      }
      case Close: {
        _connState |= WS_CLOSE_RECEIVED;

        // If we haven't sent a Close frame before, send one now, echoing
        // the callback
        if (!(_connState & WS_CLOSE_SENT)) {
          _connState |= WS_CLOSE_SENT;
          sendWSMessage(Close, &_payload[0], _payload.size());
        }

        // TODO: Delay closeWSSocket call until close message is actually sent
        closeWSSocket();

        // TODO: Use code and status
        onWSClose(0);

        break;
      }
      case Ping: {
        // Send back a pong
        sendWSMessage(Pong, &_payload[0], _payload.size());
        break;
      }
      case Pong: {
        // No action needed
        break;
      }
      case Reserved: {
        // TODO: Warn and close connection?
        break;
      }
    }
  }

  _payload.clear();
}

// trim from start
static inline std::string &ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
  return ltrim(rtrim(s));
}

std::string createHandshakeResponse(std::string key) {
  std::string clear = trim(key) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA1_CTX ctx;
  reid_SHA1_Init(&ctx);
  reid_SHA1_Update(&ctx, (uint8_t*)&clear[0], clear.size());

  std::vector<uint8_t> digest(SHA1_DIGEST_SIZE);
  reid_SHA1_Final(&ctx, &digest[0]);

  return b64encode(digest);
}

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

void createFrameHeader(Opcode opcode, bool mask, size_t payloadSize,
                       int32_t maskingKey,
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
