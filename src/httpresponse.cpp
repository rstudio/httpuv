#include "httpresponse.h"
#include "httprequest.h"
#include "constants.h"
#include "thread.h"
#include "utils.h"
#include "libuv/include/uv.h"


void on_response_written(uv_write_t* handle, int status) {
  ASSERT_BACKGROUND_THREAD()
  // Make a local copy of the shared_ptr before deleting the original one.
  boost::shared_ptr<HttpResponse> pResponse(*(boost::shared_ptr<HttpResponse>*)handle->data);

  delete (boost::shared_ptr<HttpResponse>*)handle->data;
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
  boost::shared_ptr<HttpResponse> _pParent;
public:
  HttpResponseExtendedWrite(boost::shared_ptr<HttpResponse> pParent,
                            uv_stream_t* pHandle,
                            boost::shared_ptr<DataSource> pDataSource) :
      ExtendedWrite(pHandle, pDataSource), _pParent(pParent) {}

  void onWriteComplete(int status) {
    delete this;
  }
};

void HttpResponse::writeResponse() {
  ASSERT_BACKGROUND_THREAD()
  debug_log("HttpResponse::writeResponse", DEBUG);
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
      _pBody.reset();
    }
  }

  uv_buf_t headerBuf = uv_buf_init(safe_vec_addr(_responseHeader), _responseHeader.size());
  uv_write_t* pWriteReq = (uv_write_t*)malloc(sizeof(uv_write_t));
  memset(pWriteReq, 0, sizeof(uv_write_t));
  // Pointer to shared_ptr
  pWriteReq->data = new boost::shared_ptr<HttpResponse>(shared_from_this());

  int r = uv_write(pWriteReq, _pRequest->handle(), &headerBuf, 1,
      &on_response_written);
  if (r) {
    debug_log(std::string("uv_write() error:") + uv_strerror(r), INFO);
    delete (boost::shared_ptr<HttpResponse>*)pWriteReq->data;
    free(pWriteReq);
  } else {
    _pRequest->requestCompleted();
  }
}

void HttpResponse::onResponseWritten(int status) {
  ASSERT_BACKGROUND_THREAD()
  debug_log("HttpResponse::onResponseWritten", DEBUG);
  if (status != 0) {
    err_printf("Error writing response: %d\n", status);
    _closeAfterWritten = true; // Cause the request connection to close.
    return;
  }

  if (_pBody != NULL) {
    HttpResponseExtendedWrite* pResponseWrite = new HttpResponseExtendedWrite(
      shared_from_this(), _pRequest->handle(), _pBody);
    pResponseWrite->begin();
  }
}

// This sets a flag so that the connection is closed after the response is
// written. It also adds a "Connection: close" header to the response.
void HttpResponse::closeAfterWritten() {
  setHeader("Connection", "close");
  _closeAfterWritten = true;
}

HttpResponse::~HttpResponse() {
  ASSERT_BACKGROUND_THREAD()
  debug_log("HttpResponse::~HttpResponse", DEBUG);
  if (_closeAfterWritten) {
    _pRequest->close();
  }
  _pBody.reset();
}
