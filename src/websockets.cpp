#include "websockets.h"
#include "utils.h"
#include "thread.h"
#include <assert.h>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>

#include "sha1/sha1.h"
#include "base64/base64.hpp"

#include "websockets-ietf.h"
#include "websockets-hybi03.h"
#include "websockets-hixie76.h"

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

bool WSHyBiFrameHeader::isHeaderComplete() const {
  if (_data.size() < 2)
    return false;

  return _data.size() >= (size_t)headerLength();
}

WSFrameHeaderInfo WSHyBiFrameHeader::info() const {
  WSFrameHeaderInfo inf;
  inf.fin = fin();
  inf.opcode = opcode();
  inf.hasLength = true;
  inf.masked = masked();
  if (masked()) {
    inf.maskingKey.resize(4);
    maskingKey(safe_vec_addr(inf.maskingKey));
  }
  inf.payloadLength = payloadLength();
  return inf;
}

bool WSHyBiFrameHeader::fin() const {
  return _pProto->isFin(read(0, 1));
}
Opcode WSHyBiFrameHeader::opcode() const {
  uint8_t oc = read(4, 4);
  return _pProto->decodeOpcode(oc);
}
bool WSHyBiFrameHeader::masked() const {
  return read(8, 1) != 0;
}
uint64_t WSHyBiFrameHeader::payloadLength() const {
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
void WSHyBiFrameHeader::maskingKey(uint8_t key[4]) const {
  if (!masked())
    memset(key, 0, 4);
  else {
    key[0] = read(9 + payloadLengthLength(), 8);
    key[1] = read(9 + payloadLengthLength() + 8, 8);
    key[2] = read(9 + payloadLengthLength() + 16, 8);
    key[3] = read(9 + payloadLengthLength() + 24, 8);
  }
}
size_t WSHyBiFrameHeader::headerLength() const {
  return (9 + payloadLengthLength() + maskingKeyLength()) / 8;
}
uint8_t WSHyBiFrameHeader::read(size_t bitOffset, size_t bitWidth) const {
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
uint64_t WSHyBiFrameHeader::read64(size_t bitOffset, size_t bitWidth) const {
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
uint8_t WSHyBiFrameHeader::payloadLengthLength() const {
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
uint8_t WSHyBiFrameHeader::maskingKeyLength() const {
  return masked() ? 32 : 0;
}

void WSHyBiParser::handshake(const std::string& url,
                             const RequestHeaders& requestHeaders,
                             char** ppData, size_t* pLen,
                             ResponseHeaders* pResponseHeaders,
                             std::vector<uint8_t>* pResponse) const {
  ASSERT_BACKGROUND_THREAD()
  _pProto->handshake(url, requestHeaders, ppData, pLen, pResponseHeaders,
                     pResponse);
}

void WSHyBiParser::createFrameHeaderFooter(
                     Opcode opcode, bool mask, size_t payloadSize,
                     int32_t maskingKey,
                     char pHeaderData[MAX_HEADER_BYTES], size_t* pHeaderLen,
                     char pFooterData[MAX_FOOTER_BYTES], size_t* pFooterLen
                     ) const {
  _pProto->createFrameHeader(opcode, mask, payloadSize, maskingKey,
                             pHeaderData, pHeaderLen);
}

void WSHyBiParser::read(const char* data, size_t len) {
  ASSERT_BACKGROUND_THREAD()
  while (len > 0) {
    // crude check for underflow
    assert(len < 1000000000000000000);

    switch (_state) {
      case InHeader: {
        // The _header vector<char> accumulates header data until
        // the complete header is read. It's possible/likely it also
        // holds part of the payload.
        size_t startingSize = _header.size();
        std::copy(data, data + min(len, MAX_HEADER_BYTES - startingSize),
          std::back_inserter(_header));

        WSHyBiFrameHeader frame(_pProto, safe_vec_addr(_header), _header.size());

        if (frame.isHeaderComplete()) {
          _pCallbacks->onHeaderComplete(frame.info());

          size_t payloadOffset = frame.headerLength() - startingSize;
          _bytesLeft = frame.payloadLength();

          _state = InPayload;
          _header.clear();

          data += payloadOffset;
          len -= payloadOffset;
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
        _pCallbacks->onPayload(data, bytesToConsume);

        data += bytesToConsume;
        len -= bytesToConsume;

        if (_bytesLeft == 0) {
          _pCallbacks->onFrameComplete();

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

bool WebSocketConnection::accept(const RequestHeaders& requestHeaders,
                                 const char* pData, size_t len) {
  ASSERT_BACKGROUND_THREAD()
  assert(!_pParser);
  if (_connState == WS_CLOSED) return false;

  WebSocketProto_IETF ietf;
  if (ietf.canHandle(requestHeaders, pData, len)) {
    _pParser = new WSHyBiParser(this, new WebSocketProto_IETF());
    return true;
  }

  WebSocketProto_HyBi03 hybi03;
  if (hybi03.canHandle(requestHeaders, pData, len)) {
    _pParser = new WSHixie76Parser(this);
    return true;
  }
  return false;
}

void WebSocketConnection::handshake(const std::string& url,
                                    const RequestHeaders& requestHeaders,
                                    char** ppData, size_t* pLen,
                                    ResponseHeaders* pResponseHeaders,
                                    std::vector<uint8_t>* pResponse) {
  ASSERT_BACKGROUND_THREAD()
  assert(_pParser);
  if (_connState == WS_CLOSED) return;

  _pParser->handshake(url, requestHeaders, ppData, pLen, pResponseHeaders,
                      pResponse);
}

void WebSocketConnection::sendWSMessage(Opcode opcode, const char* pData, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  if (_connState == WS_CLOSED) return;

  std::vector<char> header(MAX_HEADER_BYTES);
  std::vector<char> footer(MAX_FOOTER_BYTES);

  size_t headerLength = 0;
  size_t footerLength = 0;

  _pParser->createFrameHeaderFooter(opcode, false, length, 0,
    safe_vec_addr(header), &headerLength,
    safe_vec_addr(footer), &footerLength);
  header.resize(headerLength);
  footer.resize(footerLength);

  _pCallbacks->sendWSFrame(safe_vec_addr(header), header.size(),
                           pData, length,
                           safe_vec_addr(footer), footer.size());
}

void WebSocketConnection::closeWS(uint16_t code, std::string reason) {
  ASSERT_BACKGROUND_THREAD()
  trace("WebSocketConnection::closeWS");

  switch (_connState) {
  // If we have already sent a close message, do nothing.
  case WS_CLOSE_SENT:
  case WS_CLOSED:
    return;
  case WS_OPEN:
    _connState = WS_CLOSE_SENT;
    break;
  case WS_CLOSE_RECEIVED:
    _connState = WS_CLOSED;
    break;
  }

  // Make sure code has right endian-ness
  unsigned char* code_p = (unsigned char*)&code;
  if (!isBigEndian())
    swapByteOrder(code_p, code_p + 2);

  std::string message = std::string(reinterpret_cast<char*>(code_p), 2) + reason;

  sendWSMessage(Close, message.c_str(), message.length());

  // If close messages have been both sent and received, close socket.
  if (_connState == WS_CLOSED)
    _pCallbacks->closeWSSocket();
}

void WebSocketConnection::read(const char* data, size_t len) {
  ASSERT_BACKGROUND_THREAD()
  if (_connState == WS_CLOSED) return;
  assert(_pParser);
  _pParser->read(data, len);
}

void WebSocketConnection::read(boost::shared_ptr<std::vector<char> > buf) {
  ASSERT_BACKGROUND_THREAD()
  if (_connState == WS_CLOSED) return;
  read(safe_vec_addr(*buf), buf->size());
}

void WebSocketConnection::markClosed() {
  ASSERT_BACKGROUND_THREAD()
  _connState = WS_CLOSED;
}

void WebSocketConnection::onHeaderComplete(const WSFrameHeaderInfo& header) {
  ASSERT_BACKGROUND_THREAD()
  if (_connState == WS_CLOSED) return;

  _header = header;
  if (!header.fin && header.opcode != Continuation)
    _incompleteContentHeader = header;
}
void WebSocketConnection::onPayload(const char* data, size_t len) {
  ASSERT_BACKGROUND_THREAD()
  if (_connState == WS_CLOSED) return;

  size_t origSize = _payload.size();
  std::copy(data, data + len, std::back_inserter(_payload));

  if (_header.masked != 0) {
    for (size_t i = origSize; i < _payload.size(); i++) {
      size_t j = i % 4;
      _payload[i] = _payload[i] ^ _header.maskingKey[j];
    }
  }
}
void WebSocketConnection::onFrameComplete() {
  ASSERT_BACKGROUND_THREAD()
  trace("WebSocketConnection::onFrameComplete");
  if (_connState == WS_CLOSED) return;

  if (!_header.fin) {
    std::copy(_payload.begin(), _payload.end(),
      std::back_inserter(_incompleteContentPayload));
  } else {
    switch (_header.opcode) {
      case Continuation: {
        std::copy(_payload.begin(), _payload.end(),
          std::back_inserter(_incompleteContentPayload));
        _pCallbacks->onWSMessage(_incompleteContentHeader.opcode == Binary,
          safe_vec_addr(_incompleteContentPayload), _incompleteContentPayload.size());

        _incompleteContentPayload.clear();
        break;
      }
      case Text:
      case Binary: {
        _pCallbacks->onWSMessage(_header.opcode == Binary, safe_vec_addr(_payload), _payload.size());
        break;
      }
      case Close: {

        if (_connState == WS_OPEN) {
          _connState = WS_CLOSE_RECEIVED;
        } else if (_connState == WS_CLOSE_SENT) {
          _connState = WS_CLOSED;
        }
        
        // If we haven't sent a Close frame before, send one now, echoing
        // the callback
        if (_connState != WS_CLOSE_SENT && _connState != WS_CLOSED) {
          _connState = WS_CLOSED;
          sendWSMessage(Close, safe_vec_addr(_payload), _payload.size());
        }

        // TODO: Delay closeWSSocket call until close message is actually sent
        _pCallbacks->closeWSSocket();

        // TODO: Use code and status
        _pCallbacks->onWSClose(0);

        break;
      }
      case Ping: {
        // Send back a pong
        sendWSMessage(Pong, safe_vec_addr(_payload), _payload.size());
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
