#include "http.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <algorithm>
#include <iostream>
#include <sstream>

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
  if (status != 0)
    std::cerr << "Error writing response: " << status << std::endl;
  delete ((HttpResponse*)handle->data);
  free(handle);
}

uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size) {
  // Freed in HttpRequest::_on_request_read
  void* result = malloc(suggested_size);
  return uv_buf_init((char*)result, suggested_size);
}

void HttpRequest::trace(const std::string& msg) {
  //std::cerr << msg << std::endl;
}

uv_tcp_t* HttpRequest::handle() {
  return &_handle;
}

std::string HttpRequest::method() const {
  return http_method_str((enum http_method)_parser.method);
}

std::string HttpRequest::url() const {
  return _url;
}

std::map<std::string, std::string, compare_ci> HttpRequest::headers() const {
  return _headers;
}

std::vector<char> HttpRequest::body() {
  return _body;
}

typedef struct {
  uv_write_t writeReq;
  std::vector<char>* pHeader;
  std::vector<char>* pData;
} ws_send_t;

void on_ws_message_sent(uv_write_t* handle, int status) {
  // TODO: Handle error if status != 0
  ws_send_t* pSend = (ws_send_t*)handle;
  delete pSend->pHeader;
  delete pSend->pData;
  free(pSend);
}

void HttpRequest::sendWSMessage(bool binary, const char* pData, size_t length) {
  ws_send_t* pSend = (ws_send_t*)malloc(sizeof(ws_send_t));
  memset(pSend, 0, sizeof(ws_send_t));
  pSend->pHeader = new std::vector<char>(MAX_HEADER_BYTES);
  pSend->pData = new std::vector<char>(pData, pData + length);

  size_t headerLength;
  createFrameHeader(binary ? Binary : Text, false, length, 0,
    &(*pSend->pHeader)[0], &headerLength);
  pSend->pHeader->resize(headerLength);

  uv_buf_t buffers[2];
  buffers[0] = uv_buf_init(&(*pSend->pHeader)[0], pSend->pHeader->size());
  buffers[1] = uv_buf_init(&(*pSend->pData)[0], pSend->pData->size());

  // TODO: Handle return code
  uv_write(&pSend->writeReq, (uv_stream_t*)handle(), buffers, 2,
           &on_ws_message_sent);
}

void HttpRequest::closeWS() {
  // TODO: Follow proper WS protocol (send Close frame)
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
  _lastHeaderField = std::string(pAt, length);
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
  return 0;
}

int HttpRequest::_on_headers_complete(http_parser* pParser) {
  trace("on_headers_complete");
  for (std::map<std::string, std::string, compare_ci>::iterator iter = _headers.begin();
     iter != _headers.end();
     iter++) {
    //std::cout << iter->first << std::string(" = ") << iter->second << std::endl;
  }
  // TODO: Allocate body
  return 0;
}

int HttpRequest::_on_body(http_parser* pParser, const char* pAt, size_t length) {
  trace("on_body");
  for (size_t i = 0; i < length; i++) {
    _body.push_back(*(pAt+i));
  }
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
  _pWebApplication->onWSMessage(this, binary, data, len);
}
void HttpRequest::onWSClose(int code) {
  // TODO: Call close() here?
}


void HttpRequest::fatal_error(const char* method, const char* message) {
  fprintf(stderr, "ERROR: [%s] %s\n", method, message);
}

void HttpRequest::_on_closed(uv_handle_t* handle) {
  // printf("Closed\n");
  delete this;
}

void HttpRequest::close() {
  // std::cerr << "Closing handle " << &_handle << std::endl;
  if (_protocol == WebSockets)
    _pWebApplication->onWSClose(this);
  _pSocket->removeConnection(this);
  uv_close((uv_handle_t*)&_handle, HttpRequest_on_closed);
}

void HttpRequest::_on_request_read(uv_stream_t*, ssize_t nread, uv_buf_t buf) {
  if (nread > 0) {
    //std::cerr << nread << " bytes read\n";
    if (_protocol == HTTP) {
      int parsed = http_parser_execute(&_parser, &request_settings(), buf.base, nread);
      if (_parser.upgrade) {
        char* pData = buf.base + parsed;
        ssize_t pDataLen = nread - parsed;

        if (_headers.find("upgrade") != _headers.end() &&
            _headers["upgrade"] == std::string("websocket") &&
            _headers.find("sec-websocket-key") != _headers.end()) {

          // Freed in on_response_written
          HttpResponse* pResp = new HttpResponse(this, 101, "Switching Protocols",
            std::vector<char>());
          pResp->addHeader("Upgrade", "websocket");
          pResp->addHeader("Connection", "Upgrade");
          pResp->addHeader(
            "Sec-WebSocket-Accept",
            createHandshakeResponse(_headers["sec-websocket-key"]));
          // TODO: Consult app about supported WS protocol
          //pResp->addHeader("Sec-WebSocket-Protocol", "");

          pResp->writeResponse();

          _protocol = WebSockets;
          _pWebApplication->onWSOpen(this);

          read(pData, pDataLen);
        }

        if (_protocol != WebSockets) {
          // TODO: Write failure
          close();
        }
      } else if (parsed < nread) {
        fatal_error("on_request_read", "parse error");
        close();
      }
    } else if (_protocol == WebSockets) {
      read(buf.base, nread);
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
  int r = uv_read_start((uv_stream_t*)&_handle, &on_alloc, &HttpRequest_on_request_read);
  if (r) {
    uv_err_t err = uv_last_error(_pLoop);
    fatal_error("read_start", uv_strerror(err));
    return;
  }
}

void HttpResponse::addHeader(const std::string& name, const std::string& value) {
  _headers.push_back(std::pair<std::string, std::string>(name, value));
}

void HttpResponse::writeResponse() {
  // TODO: Optimize
  std::ostringstream response(std::ios_base::binary);
  response << "HTTP/1.1 " << _statusCode << " " << _status << "\r\n";
  for (std::vector<std::pair<std::string, std::string> >::iterator it = _headers.begin();
     it != _headers.end();
     it++) {
    response << it->first << ": " << it->second << "\r\n";
  }
  response << "Content-Length: " << _bodyBuf.size() << "\r\n";
  response << "\r\n";
  std::string responseStr = response.str();
  _responseHeader.assign(responseStr.begin(), responseStr.end());

  uv_buf_t headerBuf = uv_buf_init(&_responseHeader[0], _responseHeader.size());
  uv_buf_t bodyBuf = uv_buf_init(&_bodyBuf[0], _bodyBuf.size());
  uv_buf_t buffers[] = {headerBuf, bodyBuf};

  // Freed in on_response_written
  uv_write_t* pWriteReq = (uv_write_t*)malloc(sizeof(uv_write_t));
  memset(pWriteReq, 0, sizeof(uv_write_t));
  pWriteReq->data = this;

  int r = uv_write(pWriteReq, (uv_stream_t*)this->_pRequest->handle(), buffers, 2,
                   &on_response_written);
  if (r) {
    _pRequest->fatal_error("uv_write",
                 uv_strerror(uv_last_error(_pRequest->handle()->loop)));
    delete this;
    free(pWriteReq);
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
  delete pWebApplication;
}

void Socket::destroy() {
  for (std::vector<HttpRequest*>::reverse_iterator it = connections.rbegin();
    it != connections.rend();
    it++) {

    // std::cerr << "Request close on " << *it << std::endl;
    (*it)->close();
  }
  uv_close((uv_handle_t*)&handle, on_Socket_close);
}

void on_Socket_close(uv_handle_t* pHandle) {
  delete (Socket*)pHandle->data;
}

void on_request(uv_stream_t* handle, int status) {
  if (status == -1) {
    uv_err_t err = uv_last_error(handle->loop);
    fprintf(stderr, "connection error: %s\n", uv_strerror(err));
    return;
  }

  Socket* pSocket = (Socket*)handle->data;

  // Freed by HttpRequest itself when close() is called, which
  // can occur on EOF, error, or when the Socket is destroyed
  HttpRequest* req = new HttpRequest(
    handle->loop, pSocket->pWebApplication, pSocket);

  int r = uv_accept(handle, (uv_stream_t*)req->handle());
  if (r) {
    uv_err_t err = uv_last_error(handle->loop);
    fprintf(stderr, "accept: %s\n", uv_strerror(err));
    delete req;
    return;
  }

  req->handleRequest();

}

uv_tcp_t* createServer(uv_loop_t* pLoop, const std::string& host, int port,
  WebApplication* pWebApplication) {

  // Deletes itself when destroy() is called, which occurs in freeServer()
  Socket* pSocket = new Socket();
  // TODO: Handle error
  uv_tcp_init(pLoop, &pSocket->handle);
  pSocket->handle.data = pSocket;
  pSocket->pWebApplication = pWebApplication;

  struct sockaddr_in address = uv_ip4_addr(host.c_str(), port);
  int r = uv_tcp_bind(&pSocket->handle, address);
  if (r) {
    pSocket->destroy();
    return NULL;
  }
  r = uv_listen((uv_stream_t*)&pSocket->handle, 128, &on_request);
  if (r) {
    pSocket->destroy();
    return NULL;
  }

  return &pSocket->handle;
}
void freeServer(uv_tcp_t* pHandle) {
  uv_loop_t* loop = pHandle->loop;
  Socket* pSocket = (Socket*)pHandle->data;
  pSocket->destroy();
  
  runNonBlocking(loop);
}
bool runNonBlocking(uv_loop_t* loop) {
  return uv_run(loop, UV_RUN_NOWAIT);
}
