#include "websockets.hpp"
#include <assert.h>

#include <algorithm>
#include <iostream>
#include <iomanip>

bool WSFrameHeader::isHeaderComplete() const {
  if (_data.size() < 2)
    return false;

  return _data.size() * 8 >= (size_t)headerLength();
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
      return read(16, 16);
    case 127:
      return read(16, 64);
    default:
      return pl;
  }
}
uint32_t WSFrameHeader::maskingKey() const {
  if (!masked())
    return 0;
  else
    return read64(9 + payloadLengthLength(), 32);
}
size_t WSFrameHeader::headerLength() const {
  return 9 + payloadLengthLength() + maskingKeyLength();
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
    result = (result << 8) + _data[byteOffset + i];
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
    switch (_state) {
      case InHeader: {
        // The _header vector<char> accumulates header data until
        // the complete header is read. It's possible/likely it also
        // holds part of the payload.
        size_t startingSize = _header.size();
        std::copy(data, data + std::min(len, MAX_HEADER_BYTES),
          std::back_inserter(_header));

        WSFrameHeader frame(&_header[0], _header.size());

        if (frame.isHeaderComplete()) {
          onHeaderComplete(frame);

          size_t payloadOffset = frame.headerLength() - startingSize;
          _bytesLeft = frame.payloadLength();

          _state = InPayload;
          _header.clear();

          data += payloadOffset;
          len -= payloadOffset;
        }
        break;
      }
      case InPayload: {
        size_t bytesToConsume = std::min(len, _bytesLeft);
        _bytesLeft -= bytesToConsume;
        onPayload(data, bytesToConsume);

        if (_bytesLeft == 0) {
          onFrameComplete();
          
          _state = InHeader;

          data += bytesToConsume;
          len -= bytesToConsume;
        }
        break;
      }
      default:
        assert(false);
        break;
    }
  }
}

void WebSocketConnection::onHeaderComplete(const WSFrameHeader& header) {
  _header = header;
  if (!header.fin())
    _incompleteContentHeader = header;
}
void WebSocketConnection::onPayload(const char* data, size_t len) {
  std::copy(data, data + len, std::back_inserter(_payload));
}
void WebSocketConnection::onFrameComplete() {
  if (_header.fin()) {
    switch (_header.opcode()) {
      case Text:
      case Binary: {
        onWSMessage(_header.opcode() == Binary, &_payload[0], _payload.size());
        break;
      }
      case Close: {
        // TODO: Send close response
        // TODO: Use correct close code
        onWSClose(0);
      }
      case Ping: {
        // TODO: Implement ping
      }
      case Pong: {
        // TODO: Implement pong
      }
    }

    _payload.clear();
  }
}

int main() {
  const char* data = "\x01\x7E";
  WSFrameHeader frame(data, 2);
  //std::cout << (int)frame.read(12, 4) << std::endl;
  //std::cout << (int)frame.read64(0, 16) << std::endl;
  std::cout << frame.isHeaderComplete() << std::endl;
}
