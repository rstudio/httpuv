#ifndef WEBSOCKETS_HPP
#define WEBSOCKETS_HPP

#include <string.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "constants.h"
#include "websockets-base.h"

/* Interprets the bytes that make up a WebSocket frame header.
 * See RFC 6455 Section 5 (especially 5.2) for details on the
 * wire format.
 */
class WSFrameHeader {
  std::vector<char> _data;
  WebSocketProto* _pProto;

public:
  WSFrameHeader() : _data(MAX_HEADER_BYTES) {
  }

  // The data is copied (up to 14 bytes worth)
  WSFrameHeader(WebSocketProto* pProto, const char* data, size_t len)
    : _pProto(pProto), _data(data, data + (std::min(MAX_HEADER_BYTES, len))) {
  }

  virtual ~WSFrameHeader() {
  }

  // IMPORTANT: Don't attempt to call any of the other methods
  // until isHeaderComplete is true!!
  bool isHeaderComplete() const;
  bool isPayloadComplete() const;

  bool fin() const;
  Opcode opcode() const;
  bool masked() const;
  uint64_t payloadLength() const;
  void maskingKey(uint8_t key[4]) const;
  size_t headerLength() const;

//private:
  // Read part of a byte, and interpret the bits as an unsigned number.
  // The bitOffset is starting from the most significant bit.
  // IMPORTANT: (bitOffset % 8) + bitWidth MUST be 8 or less!
  // In other words, the bits you request must not span multiple bytes.
  uint8_t read(size_t bitOffset, size_t bitWidth) const;
  // Read bytes, an interpret them as a big endian number.
  // IMPORTANT: bitOffset and bitWidth MUST be multiples of 8!
  // IMPORTANT: bitWidth MUST be 64 or less!
  uint64_t read64(size_t bitOffset, size_t bitWidth) const;
  uint8_t payloadLengthLength() const;
  uint8_t maskingKeyLength() const;
};

class WebSocketParserCallbacks {
public:
  virtual void onHeaderComplete(const WSFrameHeader& header) = 0;
  // The data is copied
  virtual void onPayload(const char* data, size_t len) = 0;
  virtual void onFrameComplete() = 0;
};

class WebSocketParser {
  WebSocketParserCallbacks* _pCallbacks;
  WSParseState _state;
  std::vector<char> _header;
  uint64_t _bytesLeft;

public:
  WebSocketParser(WebSocketParserCallbacks* callbacks)
      : _pCallbacks(callbacks), _state(InHeader) {
  }
  virtual ~WebSocketParser() {
  }

  void read(WebSocketProto* pProto, const char* data, size_t len);
};

typedef uint8_t WSConnState;
const WSConnState WS_OPEN = 0;
const WSConnState WS_CLOSE_RECEIVED = 1;
const WSConnState WS_CLOSE_SENT = 2;
const WSConnState WS_CLOSE = WS_CLOSE_RECEIVED | WS_CLOSE_SENT;

class WebSocketConnectionCallbacks {
public:
  virtual void onWSMessage(bool binary, const char* data, size_t len) = 0;
  virtual void onWSClose(int code) = 0;
  // Implementers MUST copy data
  virtual void sendWSFrame(const char* headerData, size_t headerLength,
                           const char* pData, size_t dataLength) = 0;
  virtual void closeWSSocket() = 0;
};

class WebSocketConnection : WebSocketParserCallbacks {
  WebSocketConnectionCallbacks* _pCallbacks;
  WebSocketParser* _pParser;
  WebSocketProto* _pProto;
  WSConnState _connState;
  WSFrameHeader _incompleteContentHeader;
  WSFrameHeader _header;
  std::vector<char> _incompleteContentPayload;
  std::vector<char> _payload;

public:
  WebSocketConnection(WebSocketConnectionCallbacks* callbacks)
      : _pCallbacks(callbacks), _connState(WS_OPEN),
        _pParser(new WebSocketParser(this)), _pProto(NULL) {
  }
  virtual ~WebSocketConnection() {
    delete _pParser;
    delete _pProto;
  }

  bool accept(const RequestHeaders& requestHeaders, const char* pData, size_t len);
  void handshake(const RequestHeaders& requestHeaders,
                 char* pData, size_t len,
                 ResponseHeaders* pResponseHeaders,
                 std::vector<uint8_t>* pResponse);

  void sendWSMessage(Opcode opcode, const char* pData, size_t length);
  void closeWS();
  void read(const char* data, size_t len);

protected:
  void onHeaderComplete(const WSFrameHeader& header);
  void onPayload(const char* data, size_t len);
  void onFrameComplete();
};

#endif // WEBSOCKETS_HPP
