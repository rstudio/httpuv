#ifndef WEBSOCKETS_HIXIE76_H
#define WEBSOCKETS_HIXIE76_H

#include "websockets.h"
#include "websockets-hybi03.h"

enum Hixie76State {
  // Starting state, also what we return to after finishing a frame
  H76_START,
  // We saw 0x00 indicating text frame, now looking for matching 0xFF.
  // Everything in between is payload data.
  H76_IN_TEXT_FRAME,
  // We just saw 0xFF, the next byte will determine if it's a close
  // message (i.e. if it's 0x00) or if it's a regular binary frame.
  H76_IN_BINARY_OR_CLOSE_FRAME_LENGTH,
  // The current byte is part of the frame length info; if the high-order
  // bit is set to 1, then the next byte is also part of the frame length
  // info.
  H76_IN_BINARY_FRAME_LENGTH,
  // We are in the binary payload, with _bytesLeft to go.
  H76_IN_BINARY_FRAME
};

class WSHixie76Parser : public WSParser {
private:
  WSParserCallbacks* _pCallbacks;
  WebSocketProto_HyBi03 _hybi03;
  int _state;
  size_t _bytesLeft;

public:
  WSHixie76Parser(WSParserCallbacks* pCallbacks) : _pCallbacks(pCallbacks) {
  }
  ~WSHixie76Parser() {}

  void handshake(const std::string& url,
                 const RequestHeaders& requestHeaders,
                 char** ppData, size_t* pLen,
                 ResponseHeaders* responseHeaders,
                 std::vector<uint8_t>* pResponse) const;

  void createFrameHeader(Opcode opcode, bool mask, size_t payloadSize,
                         int32_t maskingKey,
                         char pData[MAX_HEADER_BYTES], size_t* pLen) const;

  void read(const char* data, size_t len);
};

#endif // WEBSOCKETS_HIXIE76_H
