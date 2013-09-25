#include "http.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <algorithm>
#include <iostream>
#include <sstream>

#include <Rinternals.h>

// TODO: Streaming response body (with chunked transfer encoding)
// TODO: Fast/easy use of files as response body

http_parser_settings& request_settings() {
  static http_parser_settings settings;
  settings.on_message_begin = HttpRequest_on_message_begin;
  settings.on_url = HttpRequest_on_url;
  settings.on_status_complete = HttpRequest_on_status_complete;
  settings.on_header_field = HttpRequest_on_header_field;
  settings.on_header_value = HttpRequest_on_header_value;
  settings.on_headers_complete = HttpRequest_on_headers_complete;
  settings.on_body = HttpRequest_on_body;
  settings.on_message_complete = HttpRequest_on_message_complete;
  return settings;
}

void on_response_written(uv_write_t* handle, int status) {
  HttpResponse* pResponse = (HttpResponse*)handle->data;
  free(handle);
  pResponse->onResponseWritten(status);
}

uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size) {
  // Freed in HttpRequest::_on_request_read
  void* result = malloc(suggested_size);
  return uv_buf_init((char*)result, suggested_size);
}

void HttpRequest::trace(const std::string& msg) {
  //std::cerr << msg << std::endl;
}

uv_stream_t* HttpRequest::handle() {
  return &_handle.stream;
}

Address HttpRequest::serverAddress() {
  Address address;

  if (_handle.isTcp) {
    struct sockaddr_in addr = {0};
    int len = sizeof(sockaddr_in);
    int r = uv_tcp_getsockname(&_handle.tcp, (struct sockaddr*)&addr, &len);
    if (r) {
      // TODO: warn?
      return address;
    }

    if (addr.sin_family != AF_INET) {
      // TODO: warn
      return address;
    }

    // addrstr is a pointer to static buffer, no need to free
    char* addrstr = inet_ntoa(addr.sin_addr);
    if (addrstr)
      address.host = std::string(addrstr);
    else {
      // TODO: warn?
    }
    address.port = ntohs(addr.sin_port);
  }

  return address;
}

Address HttpRequest::clientAddress() {
  Address address;

  if (_handle.isTcp) {
    struct sockaddr_in addr = {0};
    int len = sizeof(sockaddr_in);
    int r = uv_tcp_getpeername(&_handle.tcp, (struct sockaddr*)&addr, &len);
    if (r) {
      // TODO: warn?
      return address;
    }

    if (addr.sin_family != AF_INET) {
      // TODO: warn
      return address;
    }

    // addrstr is a pointer to static buffer, no need to free
    char* addrstr = inet_ntoa(addr.sin_addr);
    if (addrstr)
      address.host = std::string(addrstr);
    else {
      // TODO: warn?
    }
    address.port = ntohs(addr.sin_port);
  }

  return address;
}

Rcpp::Environment& HttpRequest::env() {
  return _env;
}

std::string HttpRequest::method() const {
  return http_method_str((enum http_method)_parser.method);
}

std::string HttpRequest::url() const {
  return _url;
}

const RequestHeaders& HttpRequest::headers() const {
  return _headers;
}

typedef struct {
  uv_write_t writeReq;
  std::vector<char>* pHeader;
  std::vector<char>* pData;
  std::vector<char>* pFooter;
} ws_send_t;

void on_ws_message_sent(uv_write_t* handle, int status) {
  // TODO: Handle error if status != 0
  ws_send_t* pSend = (ws_send_t*)handle;
  delete pSend->pHeader;
  delete pSend->pData;
  delete pSend->pFooter;
  free(pSend);
}

void HttpRequest::sendWSFrame(const char* pHeader, size_t headerSize,
                              const char* pData, size_t dataSize,
                              const char* pFooter, size_t footerSize) {
  ws_send_t* pSend = (ws_send_t*)malloc(sizeof(ws_send_t));
  memset(pSend, 0, sizeof(ws_send_t));
  pSend->pHeader = new std::vector<char>(pHeader, pHeader + headerSize);
  pSend->pData = new std::vector<char>(pData, pData + dataSize);
  pSend->pFooter = new std::vector<char>(pFooter, pFooter + footerSize);

  uv_buf_t buffers[3];
  buffers[0] = uv_buf_init(&(*pSend->pHeader)[0], pSend->pHeader->size());
  buffers[1] = uv_buf_init(&(*pSend->pData)[0], pSend->pData->size());
  buffers[2] = uv_buf_init(&(*pSend->pFooter)[0], pSend->pFooter->size());

  // TODO: Handle return code
  uv_write(&pSend->writeReq, (uv_stream_t*)handle(), buffers, 3,
           &on_ws_message_sent);
}

void HttpRequest::closeWSSocket() {
  close();
}


int HttpRequest::_on_message_begin(http_parser* pParser) {
  trace("on_message_begin");
  _headers.clear();
  return 0;
}

int HttpRequest::_on_url(http_parser* pParser, const char* pAt, size_t length) {
  trace("on_url");
  _url = std::string(pAt, length);
  return 0;
}

int HttpRequest::_on_status_complete(http_parser* pParser) {
  trace("on_status_complete");
  return 0;
}
int HttpRequest::_on_header_field(http_parser* pParser, const char* pAt, size_t length) {
  trace("on_header_field");
  std::copy(pAt, pAt + length, std::back_inserter(_lastHeaderField));
  return 0;
}

int HttpRequest::_on_header_value(http_parser* pParser, const char* pAt, size_t length) {
  trace("on_header_value");

  std::string value(pAt, length);

  if (_headers.find(_lastHeaderField) != _headers.end()) {
    // If the field already exists...

    if (_headers[_lastHeaderField].size() > 0) {
      // ...and is already non-empty...

      if (value.size() > 0) {
        // ...and this value is also non-empty, then combine using comma...
        value = _headers[_lastHeaderField] + "," + value;
      } else {
        // ...but if this value is empty, then use previous value (no-op).
        value = _headers[_lastHeaderField];
      }
    }
  }

  _headers[_lastHeaderField] = value;
  _lastHeaderField.clear();
  return 0;
}

int HttpRequest::_on_headers_complete(http_parser* pParser) {
  trace("on_headers_complete");

  int result = 0;

  HttpResponse* pResp = _pWebApplication->onHeaders(this);
  if (pResp) {
    bool bodyExpected = _headers.find("Content-Length") != _headers.end() ||
      _headers.find("Transfer-Encoding") != _headers.end();

    if (bodyExpected) {
      // If we're expecting a request body and we're returning a response
      // prematurely, then add "Connection: close" header to the response and
      // set a flag to ignore all future reads on this connection.

      pResp->addHeader("Connection", "close");

      uv_read_stop((uv_stream_t*)handle());

      _ignoreNewData = true;
    }
    pResp->writeResponse();

    // result = 1 has special meaning to http_parser for this one callback; it
    // means F_SKIPBODY should be set on the parser. That's not what we want
    // here; we just want processing to terminate.
    result = 2;
  }
  else {
    // If the request is Expect: Continue, and the app didn't say otherwise,
    // then give it what it wants
    if (_headers.find("Expect") != _headers.end()
        && _headers["Expect"] == "100-continue") {
      pResp = new HttpResponse(this, 100, "Continue", NULL);
      pResp->writeResponse();
    }
  }

  // TODO: Allocate body
  return result;
}

int HttpRequest::_on_body(http_parser* pParser, const char* pAt, size_t length) {
  trace("on_body");
  _pWebApplication->onBodyData(this, pAt, length);
  _bytesRead += length;
  return 0;
}

int HttpRequest::_on_message_complete(http_parser* pParser) {
  trace("on_message_complete");

  if (!pParser->upgrade) {
    // Deleted in on_response_written
    HttpResponse* pResp = _pWebApplication->getResponse(this);
    pResp->writeResponse();
  }

  return 0;
}

void HttpRequest::onWSMessage(bool binary, const char* data, size_t len) {
  _pWebApplication->onWSMessage(_pWebSocketConnection, binary, data, len);
}
void HttpRequest::onWSClose(int code) {
  // TODO: Call close() here?
}


void HttpRequest::fatal_error(const char* method, const char* message) {
  REprintf("ERROR: [%s] %s\n", method, message);
}

void HttpRequest::_on_closed(uv_handle_t* handle) {
  // printf("Closed\n");
  delete this;
}

void HttpRequest::close() {
  // std::cerr << "Closing handle " << &_handle << std::endl;
  if (_protocol == WebSockets)
    _pWebApplication->onWSClose(_pWebSocketConnection);
  _pSocket->removeConnection(this);
  uv_close(toHandle(&_handle.stream), HttpRequest_on_closed);
}

void HttpRequest::_on_request_read(uv_stream_t*, ssize_t nread, uv_buf_t buf) {
  if (nread > 0) {
    //std::cerr << nread << " bytes read\n";
    if (_ignoreNewData) {
      // Do nothing
    } else if (_protocol == HTTP) {
      int parsed = http_parser_execute(&_parser, &request_settings(), buf.base, nread);
      if (_parser.upgrade) {
        char* pData = buf.base + parsed;
        size_t pDataLen = nread - parsed;

        if (_pWebSocketConnection->accept(_headers, pData, pDataLen)) {
          // Freed in on_response_written
          InMemoryDataSource* pDS = new InMemoryDataSource();
          HttpResponse* pResp = new HttpResponse(this, 101, "Switching Protocols",
            pDS);

          std::vector<uint8_t> body;
          _pWebSocketConnection->handshake(_url, _headers, &pData, &pDataLen,
                                           &pResp->headers(), &body);
          if (body.size() > 0) {
            pDS->add(body);
          }
          body.empty();

          pResp->writeResponse();

          _protocol = WebSockets;
          _pWebApplication->onWSOpen(this);

          _pWebSocketConnection->read(pData, pDataLen);
        }

        if (_protocol != WebSockets) {
          // TODO: Write failure
          close();
        }
      } else if (parsed < nread) {
        if (!_ignoreNewData) {
          fatal_error("on_request_read", "parse error");
          uv_read_stop((uv_stream_t*)handle());
          close();
        }
      }
    } else if (_protocol == WebSockets) {
      _pWebSocketConnection->read(buf.base, nread);
    }
  } else if (nread < 0) {
    uv_err_t err = uv_last_error(_pLoop);
    if (err.code == UV_EOF /*|| err.code == UV_ECONNRESET*/) {
    } else {
      fatal_error("on_request_read", uv_strerror(err));
    }
    close();
  } else {
    // It's normal for nread == 0, it's when uv requests a buffer then
    // decides it doesn't need it after all
  }

  free(buf.base);
}

void HttpRequest::handleRequest() {
  int r = uv_read_start(handle(), &on_alloc, &HttpRequest_on_request_read);
  if (r) {
    uv_err_t err = uv_last_error(_pLoop);
    fatal_error("read_start", uv_strerror(err));
    return;
  }
}

ResponseHeaders& HttpResponse::headers() {
  return _headers;
}

void HttpResponse::addHeader(const std::string& name, const std::string& value) {
  _headers.push_back(std::pair<std::string, std::string>(name, value));
}

class HttpResponseExtendedWrite : public ExtendedWrite {
  HttpResponse* _pParent;
public:
  HttpResponseExtendedWrite(HttpResponse* pParent, uv_stream_t* pHandle,
                            DataSource* pDataSource) :
      ExtendedWrite(pHandle, pDataSource), _pParent(pParent) {}

  void onWriteComplete(int status) {
    delete _pParent;
    delete this;
  }
};

void HttpResponse::writeResponse() {
  // TODO: Optimize
  std::ostringstream response(std::ios_base::binary);
  response << "HTTP/1.1 " << _statusCode << " " << _status << "\r\n";
  for (ResponseHeaders::const_iterator it = _headers.begin();
     it != _headers.end();
     it++) {
    response << it->first << ": " << it->second << "\r\n";
  }
  if (_pBody)
    response << "Content-Length: " << _pBody->size() << "\r\n";
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
    _pRequest->fatal_error("uv_write",
                 uv_strerror(uv_last_error(_pRequest->handle()->loop)));
    delete this;
    free(pWriteReq);
  }
}

void HttpResponse::onResponseWritten(int status) {
  if (status != 0) {
    REprintf("Error writing response: %d\n", status);
    _pRequest->close();
    delete this;
    return;
  }

  if (_pBody == NULL) {
    delete this;
  }
  else {
    HttpResponseExtendedWrite* pResponseWrite = new HttpResponseExtendedWrite(
        this, _pRequest->handle(), _pBody);
    pResponseWrite->begin();
  }
}


#define IMPLEMENT_CALLBACK_1(type, function_name, return_type, type_1) \
  return_type type##_##function_name(type_1 arg1) { \
    return ((type*)(arg1->data))->_##function_name(arg1); \
  }
#define IMPLEMENT_CALLBACK_2(type, function_name, return_type, type_1, type_2) \
  return_type type##_##function_name(type_1 arg1, type_2 arg2) { \
    return ((type*)(arg1->data))->_##function_name(arg1, arg2); \
  }
#define IMPLEMENT_CALLBACK_3(type, function_name, return_type, type_1, type_2, type_3) \
  return_type type##_##function_name(type_1 arg1, type_2 arg2, type_3 arg3) { \
    return ((type*)(arg1->data))->_##function_name(arg1, arg2, arg3); \
  }

IMPLEMENT_CALLBACK_1(HttpRequest, on_message_begin, int, http_parser*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_url, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(HttpRequest, on_status_complete, int, http_parser*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_header_field, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_3(HttpRequest, on_header_value, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(HttpRequest, on_headers_complete, int, http_parser*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_body, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(HttpRequest, on_message_complete, int, http_parser*)
IMPLEMENT_CALLBACK_1(HttpRequest, on_closed, void, uv_handle_t*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_request_read, void, uv_stream_t*, ssize_t, uv_buf_t)

void on_Socket_close(uv_handle_t* pHandle);

void Socket::addConnection(HttpRequest* request) {
  connections.push_back(request);
}

void Socket::removeConnection(HttpRequest* request) {
  connections.erase(
    std::remove(connections.begin(), connections.end(), request),
    connections.end());
}

Socket::~Socket() {
  try {
    delete pWebApplication;
  } catch(...) {}
}

void Socket::destroy() {
  for (std::vector<HttpRequest*>::reverse_iterator it = connections.rbegin();
    it != connections.rend();
    it++) {

    // std::cerr << "Request close on " << *it << std::endl;
    (*it)->close();
  }
  uv_close(toHandle(&handle.stream), on_Socket_close);
}

void on_Socket_close(uv_handle_t* pHandle) {
  delete (Socket*)pHandle->data;
}

void on_request(uv_stream_t* handle, int status) {
  if (status == -1) {
    uv_err_t err = uv_last_error(handle->loop);
    REprintf("connection error: %s\n", uv_strerror(err));
    return;
  }

  Socket* pSocket = (Socket*)handle->data;

  // Freed by HttpRequest itself when close() is called, which
  // can occur on EOF, error, or when the Socket is destroyed
  HttpRequest* req = new HttpRequest(
    handle->loop, pSocket->pWebApplication, pSocket);

  int r = uv_accept(handle, req->handle());
  if (r) {
    uv_err_t err = uv_last_error(handle->loop);
    REprintf("accept: %s\n", uv_strerror(err));
    delete req;
    return;
  }

  req->handleRequest();

}

uv_stream_t* createPipeServer(uv_loop_t* pLoop, const std::string& name,
  int mask, WebApplication* pWebApplication) {

  // We own pWebApplication. It will be destroyed by the socket but if in
  // the future we have failure cases that stop execution before we get
  // that far, we MUST delete pWebApplication ourselves.

  // Deletes itself when destroy() is called, which occurs in freeServer()
  Socket* pSocket = new Socket();
  // TODO: Handle error
  uv_pipe_init(pLoop, &pSocket->handle.pipe, true);
  pSocket->handle.isTcp = false;
  pSocket->handle.stream.data = pSocket;
  pSocket->pWebApplication = pWebApplication;

  mode_t oldMask = 0;
  if (mask >= 0)
    oldMask = umask(mask);
  int r = uv_pipe_bind(&pSocket->handle.pipe, name.c_str());
  if (mask >= 0)
    umask(oldMask);

  if (r) {
    pSocket->destroy();
    return NULL;
  }
  r = uv_listen((uv_stream_t*)&pSocket->handle.stream, 128, &on_request);
  if (r) {
    pSocket->destroy();
    return NULL;
  }

  return &pSocket->handle.stream;
}

uv_stream_t* createTcpServer(uv_loop_t* pLoop, const std::string& host,
  int port, WebApplication* pWebApplication) {

  // We own pWebApplication. It will be destroyed by the socket but if in
  // the future we have failure cases that stop execution before we get
  // that far, we MUST delete pWebApplication ourselves.

  // Deletes itself when destroy() is called, which occurs in freeServer()
  Socket* pSocket = new Socket();
  // TODO: Handle error
  uv_tcp_init(pLoop, &pSocket->handle.tcp);
  pSocket->handle.isTcp = true;
  pSocket->handle.stream.data = pSocket;
  pSocket->pWebApplication = pWebApplication;

  struct sockaddr_in address = uv_ip4_addr(host.c_str(), port);
  int r = uv_tcp_bind(&pSocket->handle.tcp, address);
  if (r) {
    pSocket->destroy();
    return NULL;
  }
  r = uv_listen((uv_stream_t*)&pSocket->handle.stream, 128, &on_request);
  if (r) {
    pSocket->destroy();
    return NULL;
  }

  return &pSocket->handle.stream;
}
void freeServer(uv_stream_t* pHandle) {
  uv_loop_t* loop = pHandle->loop;
  Socket* pSocket = (Socket*)pHandle->data;
  pSocket->destroy();

  runNonBlocking(loop);
}
bool runNonBlocking(uv_loop_t* loop) {
  return uv_run(loop, UV_RUN_NOWAIT);
}
