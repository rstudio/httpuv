#include "websockets-hixie76.h"
#include <assert.h>

void WSHixie76Parser::handshake(const std::string& url,
                                const RequestHeaders& requestHeaders,
                                char** ppData, size_t* pLen,
                                ResponseHeaders* responseHeaders,
                                std::vector<uint8_t>* pResponse) const {
  _hybi03.handshake(url, requestHeaders, ppData, pLen, responseHeaders,
                    pResponse);
}

void WSHixie76Parser::createFrameHeaderFooter(
                       Opcode opcode, bool mask, size_t payloadSize,
                       int32_t maskingKey,
                       char pHeaderData[MAX_HEADER_BYTES], size_t* pHeaderLen,
                       char pFooterData[MAX_FOOTER_BYTES], size_t* pFooterLen) const {
  pHeaderData[0] = 0;
  *pHeaderLen = 1;

  pFooterData[0] = static_cast<char>(0xFF);
  *pFooterLen = 1;
}

#include <iostream>
void WSHixie76Parser::read(const char* data, size_t len) {
  if (len == 0)
    return;
  for (const char* pos = data; pos < data + len; pos++) {
    uint8_t b = *pos;

    if (_state == H76_START) {
      _bytesLeft = 0;

      if (b == 0xFF) {
        _state = H76_IN_BINARY_OR_CLOSE_FRAME_LENGTH;
      } else if ((0x80 & b) == 0) {
        _state = H76_IN_TEXT_FRAME;

        WSFrameHeaderInfo info;
        info.fin = true;
        info.opcode = Text;
        info.masked = false;
        info.hasLength = false;
        info.payloadLength = 0;
        _pCallbacks->onHeaderComplete(info);

      } else {
        _state = H76_IN_BINARY_FRAME_LENGTH;
      }

    } else if (_state == H76_IN_TEXT_FRAME) {

      const char* endMarker = pos;
      while (endMarker < (data + len) && *endMarker != (char)0xFF) {
        endMarker++;
      }

      // endMarker is either on an end marker, or past the end of
      // the data that has been given to us.

      // In either case, pass the data (if any) to the callbacks.
      if (pos != endMarker) {
        _pCallbacks->onPayload(pos, endMarker - pos);
      }

      if (endMarker < (data + len)) {
        assert(*endMarker == (char)0xFF);
        // We encountered a marker, all done.
        _state = H76_START;
        _pCallbacks->onFrameComplete();

        // Make sure to skip over what we read
        pos = endMarker;
      } else {
        // We didn't encounter a marker, just consumed all the data.
        return;
      }

    } else if (_state == H76_IN_BINARY_OR_CLOSE_FRAME_LENGTH) {

      if (b == 0) {
        // Close up shop
        WSFrameHeaderInfo info;
        info.fin = true;
        info.opcode = Close;
        info.masked = false;
        info.hasLength = true;
        info.payloadLength = 0;
        _pCallbacks->onHeaderComplete(info);
        _pCallbacks->onFrameComplete();
      } else {
        // Take another look at this byte, now that we know it's not
        // a close directive.
        pos--;
        _state = H76_IN_BINARY_FRAME_LENGTH;
      }

    } else if (_state == H76_IN_BINARY_FRAME_LENGTH) {

      // Add the 7 lower bits to the accumulator
      _bytesLeft *= 128;
      _bytesLeft += (b & 0x7F);

      // TODO: Detect pathologically large lengths

      // If high-order bit is set, don't continue
      if ((b & 0x80) == 0) {

        _state = H76_IN_BINARY_FRAME;

        WSFrameHeaderInfo info;
        info.fin = true;
        info.opcode = Binary;
        info.masked = false;
        info.hasLength = true;
        info.payloadLength = _bytesLeft;
        _pCallbacks->onHeaderComplete(info);

        if (_bytesLeft == 0) {
          _pCallbacks->onFrameComplete();
          _state = H76_START;
        }
      }

    } else if (_state == H76_IN_BINARY_FRAME) {

      size_t bytesToRead = len - (pos - data);
      if (bytesToRead > _bytesLeft)
        bytesToRead = _bytesLeft;

      _bytesLeft -= bytesToRead;
      _pCallbacks->onPayload(pos, bytesToRead);

      pos += bytesToRead - 1; // -1 is to compensate for pos++ in for loop

      if (_bytesLeft == 0) {
        _pCallbacks->onFrameComplete();
        _state = H76_START;
      }
    }
  }
}
