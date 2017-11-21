#include "httpresponse.h"
#include "httprequest.h"
#include "constants.h"
#include "debug.h"
#include <uv.h>


void on_response_written(uv_write_t* handle, int status) {
  ASSERT_BACKGROUND_THREAD()
  HttpResponse* pResponse = (HttpResponse*)handle->data;
  free(handle);
  pResponse->onResponseWritten(status);
}


ResponseHeaders& HttpResponse::headers() {
  return _headers;
}

void HttpResponse::addHeader(const std::string& name, const std::string& value) {
  _headers.push_back(std::pair<std::string, std::string>(name, value));
}

// Set a header to a particular value. If the header already exists, delete
// it, and add the header with the new value. The new header will be the last
// item.
void HttpResponse::setHeader(const std::string& name, const std::string& value) {
  // Look for existing header with same name, and delete if present
  ResponseHeaders::iterator it = _headers.begin();
  while (it != _headers.end()) {
    if (strcasecmp(it->first.c_str(), name.c_str()) == 0) {
      it = _headers.erase(it);
    } else {
      ++it;
    }
  }

  addHeader(name, value);
}

class HttpResponseExtendedWrite : public ExtendedWrite {
  HttpResponse* _pParent;
public:
  HttpResponseExtendedWrite(HttpResponse* pParent, uv_stream_t* pHandle,
                            DataSource* pDataSource) :
      ExtendedWrite(pHandle, pDataSource), _pParent(pParent) {}

  void onWriteComplete(int status) {
    _pParent->destroy();
    delete this;
  }
};

void HttpResponse::writeResponse() {
  ASSERT_BACKGROUND_THREAD()
  // TODO: Optimize
  std::ostringstream response(std::ios_base::binary);
  response << "HTTP/1.1 " << _statusCode << " " << _status << "\r\n";
  bool hasContentLengthHeader = false;
  for (ResponseHeaders::const_iterator it = _headers.begin();
     it != _headers.end();
     it++) {
    response << it->first << ": " << it->second << "\r\n";

    if (strcasecmp(it->first.c_str(), "Content-Length") == 0) {
      hasContentLengthHeader = true;
    }
  }

  if (_pBody && !hasContentLengthHeader) {
    response << "Content-Length: " << _pBody->size() << "\r\n";
  }

  response << "\r\n";
  std::string responseStr = response.str();
  _responseHeader.assign(responseStr.begin(), responseStr.end());

  // For Hixie-76 and HyBi-03, it's important that the body be sent immediately,
  // before any WebSocket traffic is sent from the server
  if (_statusCode == 101 && _pBody != NULL && _pBody->size() > 0 && _pBody->size() < 256) {
    uv_buf_t buffer = _pBody->getData(_pBody->size());
    if (buffer.len > 0) {
      _responseHeader.reserve(_responseHeader.size() + buffer.len);
    }
    _responseHeader.insert(_responseHeader.end(), buffer.base, buffer.base + buffer.len);
    if (buffer.len == _pBody->size()) {
      // We used up the body, kill it
      delete _pBody;
      _pBody = NULL;
    }
  }

  uv_buf_t headerBuf = uv_buf_init(&_responseHeader[0], _responseHeader.size());
  uv_write_t* pWriteReq = (uv_write_t*)malloc(sizeof(uv_write_t));
  memset(pWriteReq, 0, sizeof(uv_write_t));
  pWriteReq->data = this;

  int r = uv_write(pWriteReq, _pRequest->handle(), &headerBuf, 1,
      &on_response_written);
  if (r) {
    _pRequest->fatal_error("uv_write", uv_strerror(r));
    destroy();
    free(pWriteReq);
  }
}

void HttpResponse::onResponseWritten(int status) {
  ASSERT_BACKGROUND_THREAD()
  if (status != 0) {
    REprintf("Error writing response: %d\n", status);
    destroy(true);  // Force close
    return;
  }

  if (_pBody == NULL) {
    destroy();
  }
  else {
    HttpResponseExtendedWrite* pResponseWrite = new HttpResponseExtendedWrite(
        this, _pRequest->handle(), _pBody);
    pResponseWrite->begin();
  }
}

// This sets a flag so that the connection is closed after the response is
// written. It also adds a "Connection: close" header to the response.
void HttpResponse::closeAfterWritten() {
  setHeader("Connection", "close");
  _closeAfterWritten = true;
}

// A wrapper function that closes the HttpRequest's connection if
// closeAfterWritten() has been called or if forceClose is true, and then
// deletes this.
void HttpResponse::destroy(bool forceClose) {
  ASSERT_BACKGROUND_THREAD()
  if (forceClose || _closeAfterWritten) {
    _pRequest->close();
  }
  delete this;
}
