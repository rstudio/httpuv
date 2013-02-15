#ifndef WEBSOCKETS_HPP
#define WEBSOCKETS_HPP

#include <string.h>
#include <stdint.h>

#include <string>
#include <vector>

enum Opcode {
  Continuation = 0,
  Text = 1,
  Binary = 2,
  Close = 8,
  Ping = 9,
  Pong = 0xA,
  Reserved = 0xF
};

const size_t MAX_HEADER_BYTES = 14;

class WSFrameHeader {
  std::vector<char> _data;

public:
  WSFrameHeader() : _data(MAX_HEADER_BYTES) {
  }
  WSFrameHeader(const char* data, size_t len) 
    : _data(data, data + (std::min(MAX_HEADER_BYTES, len))) {
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

enum WSParseState {
  InHeader,
  InPayload
};

class WebSocketParser {
  WSParseState _state;
  std::vector<char> _header;
  uint64_t _bytesLeft;

public:
  WebSocketParser() : _state(InHeader) {
  }
  virtual ~WebSocketParser() {
  }

  void read(const char* data, size_t len);

protected:
  virtual void onHeaderComplete(const WSFrameHeader& header) = 0;
  virtual void onPayload(const char* data, size_t len) = 0;
  virtual void onFrameComplete() = 0;

};

class WebSocketConnection : public WebSocketParser {
  WSFrameHeader _incompleteContentHeader;
  WSFrameHeader _header;
  std::vector<char> _incompleteContentPayload;
  std::vector<char> _payload;

public:
  WebSocketConnection() {
  }
  virtual ~WebSocketConnection() {
  }

  virtual void sendWSMessage(bool binary, const char* pData, size_t length) = 0;

protected:
  virtual void onWSMessage(bool binary, const char* data, size_t len) = 0;
  virtual void onWSClose(int code) = 0;

  void onHeaderComplete(const WSFrameHeader& header);
  void onPayload(const char* data, size_t len);
  void onFrameComplete();
};

std::string createHandshakeResponse(std::string key);
void createFrameHeader(Opcode opcode, bool mask, size_t payloadSize,
                       int32_t maskingKey,
                       char ppData[MAX_HEADER_BYTES], size_t* pBytesUsed);

#endif // WEBSOCKETS_HPP
