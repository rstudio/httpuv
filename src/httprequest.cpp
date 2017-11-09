
#include <boost/bind.hpp>
#include <later_api.h>
#include "httprequest.h"
#include "callback.h"
#include "deleteobj.h"
#include "debug.h"


http_parser_settings& request_settings() {
  static http_parser_settings settings;
  settings.on_message_begin = HttpRequest_on_message_begin;
  settings.on_url = HttpRequest_on_url;
  settings.on_status = HttpRequest_on_status;
  settings.on_header_field = HttpRequest_on_header_field;
  settings.on_header_value = HttpRequest_on_header_value;
  settings.on_headers_complete = HttpRequest_on_headers_complete;
  settings.is_async_on_headers_complete = 1;
  settings.on_body = HttpRequest_on_body;
  settings.on_message_complete = HttpRequest_on_message_complete;
  return settings;
}

void on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  ASSERT_BACKGROUND_THREAD()
  // Freed in HttpRequest::_on_request_read
  void* result = malloc(suggested_size);
  *buf = uv_buf_init((char*)result, suggested_size);
}

// Does a header field `name` exist?
bool HttpRequest::_hasHeader(const std::string& name) const {
  return _headers.find(name) != _headers.end();
}

// Does a header field `name` exist and have a particular value? If ci is
// true, do a case-insensitive comparison of the value (fields are always
// case- insensitive.)
bool HttpRequest::_hasHeader(const std::string& name, const std::string& value, bool ci) const {
  RequestHeaders::const_iterator item = _headers.find(name);
  if (item == _headers.end())
    return false;

  if (ci) {
    return strcasecmp(item->second.c_str(), value.c_str()) == 0;
  } else {
    return item->second == value;
  }
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
  ASSERT_MAIN_THREAD()
  if (_env == NULL) {
    // TODO: delete this later
    _env = new Rcpp::Environment(Rcpp::Function("new.env")());
  }
  return *_env;
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

int HttpRequest::_on_message_begin(http_parser* pParser) {
  ASSERT_BACKGROUND_THREAD()
  trace("on_message_begin");
  _headers.clear();
  return 0;
}

int HttpRequest::_on_url(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  trace("on_url");
  _url = std::string(pAt, length);
  return 0;
}

int HttpRequest::_on_status(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  trace("on_status");
  return 0;
}
int HttpRequest::_on_header_field(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  trace("on_header_field");
  std::copy(pAt, pAt + length, std::back_inserter(_lastHeaderField));
  return 0;
}

int HttpRequest::_on_header_value(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
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

// ============================================================================
// Headers complete
// ============================================================================

// This is called after http-parser has finished parsing the request headers.
// It uses later() to schedule the user's R onHeaders() function. Always
// returns 0. Normally 0 indicates success for http-parser, while 1 and 2
// indicate errors or other conditions, but since we're processing the header
// asynchronously, we don't know at this point if there has been an error. If
// one of those conditions occurs, we'll set it later, but before we call
// http_parser_execute() again.
int HttpRequest::_on_headers_complete(http_parser* pParser) {
  ASSERT_BACKGROUND_THREAD()
  trace("on_headers_complete");

  boost::function<void(HttpResponse*)> schedule_bg_callback =
    boost::bind(&HttpRequest::_schedule_on_headers_complete_complete, this, _1);

  BoostFunctionCallback* webapp_on_headers_callback = new BoostFunctionCallback(
    boost::bind(
      &WebApplication::onHeaders,
      _pWebApplication,
      this,
      schedule_bg_callback
    )
  );

  // Use later to schedule _pWebApplication->onHeaders(this, schedule_bg_callback)
  // to run on the main thread. That function in turn calls
  // this->_schedule_on_headers_complete_complete.
  later::later(invoke_callback, (void*)webapp_on_headers_callback, 0);

  return 0;
}

// This is called at the end of WebApplication::onHeaders(). It puts an item
// on the write queue and signals to the background thread that there's
// something there.
void HttpRequest::_schedule_on_headers_complete_complete(HttpResponse* pResponse) {
  ASSERT_MAIN_THREAD()

  boost::function<void (void)> cb(
    boost::bind(&HttpRequest::_on_headers_complete_complete, this, pResponse)
  );

  write_queue->push(cb);
}

// This is called after the user's R onHeaders() function has finished. It can
// write a response, if onHeaders() wants that. It also sets a status code for
// http-parser and then re-executes the parser. Runs on the background thread.
void HttpRequest::_on_headers_complete_complete(HttpResponse* pResponse) {
  ASSERT_BACKGROUND_THREAD()
  trace("on_headers_complete_complete");
  int result = 0;

  if (pResponse) {
    bool bodyExpected = _hasHeader("Content-Length") || _hasHeader("Transfer-Encoding");
    bool shouldKeepAlive = http_should_keep_alive(&_parser);

    // There are two reasons we might want to send a message and close:
    // 1. If we're expecting a request body and we're returning a response
    // prematurely.
    // 2. If the parser's http_should_keep_alive() returns 0. This can happen
    // when a HTTP 1.1 request has "Connection: close". Similarly, a HTTP 1.0
    // request has that behavior as the default.
    //
    // In these cases, add "Connection: close" header to the response and
    // set a flag to ignore all future reads on this connection.
    if (bodyExpected || !shouldKeepAlive) {
      pResponse->closeAfterWritten();

      uv_read_stop((uv_stream_t*)handle());

      _ignoreNewData = true;
    }
    pResponse->writeResponse();

    // result = 1 has special meaning to http_parser for this one callback; it
    // means F_SKIPBODY should be set on the parser. That's not what we want
    // here; we just want processing to terminate, which we indicate with
    // result = 3.
    result = 3;
  }
  else {
    // If the request is Expect: Continue, and the app didn't say otherwise,
    // then give it what it wants
    if (_hasHeader("Expect", "100-continue")) {
      pResponse = new HttpResponse(this, 100, "Continue", NULL);
      pResponse->writeResponse();
    }
  }

  // Tell the parser what the result was and that it can move on.
  http_parser_headers_completed(&(this->_parser), result);

  // Continue parsing any data that went into the request buffer.
  this->_parse_http_data_from_buffer();
}


// ============================================================================
// Main message body
// ============================================================================

int HttpRequest::_on_body(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  trace("on_body");
  _pWebApplication->onBodyData(this, pAt, length);
  _bytesRead += length;
  return 0;
}

int HttpRequest::_on_message_complete(http_parser* pParser) {
  ASSERT_BACKGROUND_THREAD()
  trace("on_message_complete");

  if (pParser->upgrade)
    return 0;

  boost::function<void(HttpResponse*)> schedule_bg_callback =
    boost::bind(&HttpRequest::_schedule_on_message_complete_complete, this, _1);

  BoostFunctionCallback* webapp_get_response_callback = new BoostFunctionCallback(
    boost::bind(
      &WebApplication::getResponse,
      _pWebApplication,
      this,
      schedule_bg_callback
    )
  );

  // Use later to schedule _pWebApplication->getResponse(this, schedule_bg_callback)
  // to run on the main thread. That function in turn calls
  // this->_schedule_on_message_complete_complete.
  later::later(invoke_callback, (void*)webapp_get_response_callback, 0);

  return 0;
}

// This is called at the end of WebApplication::getResponse(). It puts an item on the
// write queue and signals to the background thread that there's something there.
void HttpRequest::_schedule_on_message_complete_complete(HttpResponse* pResponse) {
  ASSERT_MAIN_THREAD()

  boost::function<void (void)> cb(
    boost::bind(&HttpRequest::_on_message_complete_complete, this, pResponse)
  );

  write_queue->push(cb);
}

void HttpRequest::_on_message_complete_complete(HttpResponse* pResponse) {
  ASSERT_BACKGROUND_THREAD()
  trace("_on_message_complete_complete");
  if (!http_should_keep_alive(&_parser)) {
    pResponse->closeAfterWritten();

    uv_read_stop((uv_stream_t*)handle());

    _ignoreNewData = true;
  }

  pResponse->writeResponse();
}


// ============================================================================
// Incoming websocket messages
// ============================================================================

void HttpRequest::_call_r_on_ws_message(WSMessageIncoming* msg) {
  ASSERT_MAIN_THREAD()
  trace("_call_r_on_ws_message");

  _pWebApplication->onWSMessage(_pWebSocketConnection, msg->binary,
                                &(msg->data)[0], msg->len);
  delete msg;
}

void call_on_ws_message_wrapper(void* data) {
  ASSERT_MAIN_THREAD()
  WSMessageIncoming* msg = reinterpret_cast<WSMessageIncoming*>(data);
  msg->req->_call_r_on_ws_message(msg);
}

void HttpRequest::onWSMessage(bool binary, const char* data, size_t len) {
  ASSERT_BACKGROUND_THREAD()
  WSMessageIncoming* msg = new WSMessageIncoming(this, binary, data, len);
  later::later(call_on_ws_message_wrapper, msg, 0);

  // _pWebApplication->onWSMessage(_pWebSocketConnection, binary, data, len);
}
void HttpRequest::onWSClose(int code) {
  // TODO: Call close() here?
}


void HttpRequest::fatal_error(const char* method, const char* message) {
  REprintf("ERROR: [%s] %s\n", method, message);
}


// ============================================================================
// Outgoing websocket messages
// ============================================================================

typedef struct {
  uv_write_t writeReq;
  std::vector<char>* pHeader;
  std::vector<char>* pData;
  std::vector<char>* pFooter;
} ws_send_t;

void on_ws_message_sent(uv_write_t* handle, int status) {
  ASSERT_BACKGROUND_THREAD()
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
  ASSERT_BACKGROUND_THREAD()
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


// ============================================================================
// Closing connection
// ============================================================================

void HttpRequest::_on_closed(uv_handle_t* handle) {
  // printf("Closed\n");
  delete this;
}

void HttpRequest::close() {
  ASSERT_BACKGROUND_THREAD()
  // std::cerr << "Closing handle " << &_handle << std::endl;
  if (_protocol == WebSockets) {
    // Schedule:
    // _pWebApplication->onWSClose(_pWebSocketConnection)
    BoostFunctionCallback* on_ws_close_callback = new BoostFunctionCallback(
      boost::bind(
        &WebApplication::onWSClose,
        _pWebApplication,
        _pWebSocketConnection
      )
    );
    later::later(invoke_callback, (void*)on_ws_close_callback, 0);
  }

  _pSocket->removeConnection(this);

  uv_close(toHandle(&_handle.stream), HttpRequest_on_closed);
}


// ============================================================================
// Open websocket
// ============================================================================

void HttpRequest::_call_r_on_ws_open() {
  ASSERT_MAIN_THREAD()
  trace("_call_r_on_ws_open");

  this->_pWebApplication->onWSOpen(this);

  // TODO: Don't reuse requestBuffer?
  std::vector<char>* req_buffer = new std::vector<char>(_requestBuffer);
  _requestBuffer.clear();

  // Run on background thread
  boost::function<void (void)> cb(
    boost::bind(&WebSocketConnection::read,
      _pWebSocketConnection,
      &(*req_buffer)[0],
      req_buffer->size()
    )
  );

  write_queue->push(cb);
  // Free req_buffer after data is written
  write_queue->push(boost::bind(delete_obj, req_buffer));
}

void call_on_ws_open_wrapper(void* data) {
  ASSERT_MAIN_THREAD()
  HttpRequest* req = reinterpret_cast<HttpRequest*>(data);
  req->_call_r_on_ws_open();
}


// ============================================================================
// Parse incoming data
// ============================================================================

void HttpRequest::_parse_http_data(char* buffer, const ssize_t n) {
  ASSERT_BACKGROUND_THREAD()
  int parsed = http_parser_execute(&_parser, &request_settings(), buffer, n);

  if (http_parser_waiting_for_headers_completed(&_parser)) {
    // If we're waiting for the header response, just store the data in the
    // buffer.
    _requestBuffer.insert(_requestBuffer.end(), buffer + parsed, buffer + n);

  } else if (_parser.upgrade) {
    char* pData = buffer + parsed;
    size_t pDataLen = n - parsed;

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

      // TODO: Don't reuse requestBuffer?
      _requestBuffer.insert(_requestBuffer.end(), pData, pData + pDataLen);

      later::later(call_on_ws_open_wrapper, this, 0);
    }

    if (_protocol != WebSockets) {
      // TODO: Write failure
      close();
    }
  } else if (parsed < n) {
    if (!_ignoreNewData) {
      fatal_error(
        "_parse_http_data",
        http_errno_description(HTTP_PARSER_ERRNO(&_parser))
      );
      uv_read_stop((uv_stream_t*)handle());
      close();
    }
  }
}

void HttpRequest::_parse_http_data_from_buffer() {
  ASSERT_BACKGROUND_THREAD()
  // Copy contents of _requestBuffer, then clear _requestBuffer, because it
  // might be written to in _parse_http_data().
  std::vector<char> req_buffer = _requestBuffer;
  _requestBuffer.clear();

  this->_parse_http_data(&req_buffer[0], req_buffer.size());
}

void HttpRequest::_on_request_read(uv_stream_t*, ssize_t nread, const uv_buf_t* buf) {
  ASSERT_BACKGROUND_THREAD()
  if (nread > 0) {
    //std::cerr << nread << " bytes read\n";
    if (_ignoreNewData) {
      // Do nothing
    } else if (_protocol == HTTP) {
      this->_parse_http_data(buf->base, nread);

    } else if (_protocol == WebSockets) {
      _pWebSocketConnection->read(buf->base, nread);
    }
  } else if (nread < 0) {
    if (nread == UV_EOF /*|| err.code == UV_ECONNRESET*/) {
    } else {
      fatal_error("on_request_read", uv_strerror(nread));
    }
    close();
  } else {
    // It's normal for nread == 0, it's when uv requests a buffer then
    // decides it doesn't need it after all
  }

  free(buf->base);
}

void HttpRequest::handleRequest() {
  ASSERT_BACKGROUND_THREAD()
  int r = uv_read_start(handle(), &on_alloc, &HttpRequest_on_request_read);
  if (r) {
    fatal_error("read_start", uv_strerror(r));
    return;
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
IMPLEMENT_CALLBACK_3(HttpRequest, on_status, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_3(HttpRequest, on_header_field, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_3(HttpRequest, on_header_value, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(HttpRequest, on_headers_complete, int, http_parser*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_body, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(HttpRequest, on_message_complete, int, http_parser*)
IMPLEMENT_CALLBACK_1(HttpRequest, on_closed, void, uv_handle_t*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_request_read, void, uv_stream_t*, ssize_t, const uv_buf_t*)
