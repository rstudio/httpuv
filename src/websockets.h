#ifndef WEBSOCKETS_HPP
#define WEBSOCKETS_HPP

#include <string.h>
#include <stdint.h>

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "utils.h"
#include "thread.h"
#include "constants.h"
#include "websockets-base.h"

class WSFrameHeaderInfo {
public:
  bool fin;
  Opcode opcode;
  bool masked;
  std::vector<uint8_t> maskingKey;
  bool hasLength;
  uint64_t payloadLength;
};

/* Interprets the bytes that make up a WebSocket frame header.
 * See RFC 6455 Section 5 (especially 5.2) for details on the
 * wire format.
 */
class WSHyBiFrameHeader {
  std::vector<char> _data;
  WebSocketProto* _pProto;

public:
  WSHyBiFrameHeader() : _data(MAX_HEADER_BYTES), _pProto(NULL) {
  }

  // The data is copied (up to 14 bytes worth)
  WSHyBiFrameHeader(WebSocketProto* pProto, const char* data, size_t len)
    : _data(data, data + (std::min(MAX_HEADER_BYTES, len))), _pProto(pProto) {
  }

  virtual ~WSHyBiFrameHeader() {
  }

  // IMPORTANT: Don't attempt to call any of the other methods
  // until isHeaderComplete is true!!
  bool isHeaderComplete() const;
  bool isPayloadComplete() const;

  WSFrameHeaderInfo info() const;
  uint64_t payloadLength() const;
  size_t headerLength() const;

private:

  bool fin() const;
  Opcode opcode() const;
  bool masked() const;
  void maskingKey(uint8_t key[4]) const;

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

class WSParserCallbacks {
public:
  virtual void onHeaderComplete(const WSFrameHeaderInfo& header) = 0;
  // The data is copied
  virtual void onPayload(const char* data, size_t len) = 0;
  virtual void onFrameComplete() = 0;
};

class WSParser {
public:
  virtual ~WSParser() {}

  // Populate response headers with the appropriate values. This call
  // must not fail, but it will not be called unless canHandle returned
  // true previously, so any validation should be done in canHandle.
  virtual void handshake(const std::string& url,
                         const RequestHeaders& requestHeaders,
                         char** ppData, size_t* pLen,
                         ResponseHeaders* responseHeaders,
                         std::vector<uint8_t>* pResponse) const = 0;

  virtual void createFrameHeaderFooter(
                         Opcode opcode, bool mask, size_t payloadSize,
                         int32_t maskingKey,
                         char pHeaderData[MAX_HEADER_BYTES], size_t* pHeaderLen,
                         char pFooterData[MAX_FOOTER_BYTES], size_t* pFooterLen
                         ) const = 0;

  virtual void read(const char* data, size_t len) = 0;
};

class WSHyBiParser : public WSParser {
  WSParserCallbacks* _pCallbacks;
  WebSocketProto* _pProto;
  WSParseState _state;
  std::vector<char> _header;
  uint64_t _bytesLeft;

public:
  WSHyBiParser(WSParserCallbacks* callbacks, WebSocketProto* pProto)
      : _pCallbacks(callbacks), _pProto(pProto), _state(InHeader) {
  }
  virtual ~WSHyBiParser() {
    try {
      delete _pProto;
    } catch(...) {}
  }

  void handshake(const std::string& url,
                 const RequestHeaders& requestHeaders,
                 char** ppData, size_t* pLen,
                 ResponseHeaders* responseHeaders,
                 std::vector<uint8_t>* pResponse) const;

  void createFrameHeaderFooter(
                         Opcode opcode, bool mask, size_t payloadSize,
                         int32_t maskingKey,
                         char pHeaderData[MAX_HEADER_BYTES], size_t* pHeaderLen,
                         char pFooterData[MAX_FOOTER_BYTES], size_t* pFooterLen
                         ) const;

  void read(const char* data, size_t len);
};


enum WSConnState {
  WS_OPEN,
  WS_CLOSE_RECEIVED,
  WS_CLOSE_SENT,
  // This can represent two cases:
  //   1. When a close message is received, and a close message is sent.
  //   2. When the connection was simply closed without any messages.
  // It may be useful in the future to split this up.
  WS_CLOSED
};

class WebSocketConnectionCallbacks {
public:
  virtual void onWSMessage(bool binary, const char* data, size_t len) = 0;
  virtual void onWSClose(int code) = 0;
  // Implementers MUST copy data
  virtual void sendWSFrame(const char* headerData, size_t headerLength,
                           const char* pData, size_t dataLength,
                           const char* footerData, size_t footerLength) = 0;
  virtual void closeWSSocket() = 0;
};

class WebSocketConnection : WSParserCallbacks, NoCopy {
  WSConnState _connState;
  boost::shared_ptr<WebSocketConnectionCallbacks> _pCallbacks;
  WSParser* _pParser;
  WSFrameHeaderInfo _incompleteContentHeader;
  WSFrameHeaderInfo _header;
  std::vector<char> _incompleteContentPayload;
  std::vector<char> _payload;

public:
  WebSocketConnection(boost::shared_ptr<WebSocketConnectionCallbacks> callbacks)
      : _connState(WS_OPEN),
        _pCallbacks(callbacks),
        _pParser(NULL) {
  }
  virtual ~WebSocketConnection() {
    ASSERT_BACKGROUND_THREAD()
    trace("WebSocketConnection::~WebSocketConnection");
    try {
      delete _pParser;
    } catch(...) {}
  }

  bool accept(const RequestHeaders& requestHeaders, const char* pData, size_t len);
  void handshake(const std::string& url,
                 const RequestHeaders& requestHeaders,
                 char** ppData, size_t* pLen,
                 ResponseHeaders* pResponseHeaders,
                 std::vector<uint8_t>* pResponse);

  void sendWSMessage(Opcode opcode, const char* pData, size_t length);
  void closeWS(uint16_t code = 1000, std::string reason = "");
  void read(const char* data, size_t len);
  void read(boost::shared_ptr<std::vector<char> > buf);
  void markClosed();

protected:
  void onHeaderComplete(const WSFrameHeaderInfo& header);
  void onPayload(const char* data, size_t len);
  void onFrameComplete();
};

#endif // WEBSOCKETS_HPP
