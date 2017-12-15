
#include <boost/bind.hpp>
#include <later_api.h>
#include "httprequest.h"
#include "callback.h"
#include "utils.h"
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
  using namespace Rcpp;

  if (_env == NULL) {
    Environment base(R_BaseEnv);
    Function new_env = as<Function>(base["new.env"]);

    // Deleted in destructor
    _env = new Environment(new_env(_["parent"] = R_EmptyEnv));
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

void HttpRequest::responseScheduled() {
  ASSERT_MAIN_THREAD()
  trace("HttpRequest::responseScheduled");
  _response_scheduled = true;
}

bool HttpRequest::isResponseScheduled() {
  ASSERT_MAIN_THREAD()
  return _response_scheduled;
}


// ============================================================================
// Miscellaneous callbacks for http parser
// ============================================================================

int HttpRequest::_on_message_begin(http_parser* pParser) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_message_begin");
  _headers.clear();
  return 0;
}

int HttpRequest::_on_url(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_url");
  _url = std::string(pAt, length);
  return 0;
}

int HttpRequest::_on_status(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_status");
  return 0;
}
int HttpRequest::_on_header_field(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_header_field");
  std::copy(pAt, pAt + length, std::back_inserter(_lastHeaderField));
  return 0;
}

int HttpRequest::_on_header_value(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_header_value");

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
  trace("HttpRequest::_on_headers_complete");

  boost::function<void(HttpResponse*)> schedule_bg_callback(
    boost::bind(&HttpRequest::_schedule_on_headers_complete_complete, this, _1)
  );

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

  _increment_reference();

  return 0;
}

// This is called at the end of WebApplication::onHeaders(). It puts an item
// on the write queue and signals to the background thread that there's
// something there.
void HttpRequest::_schedule_on_headers_complete_complete(HttpResponse* pResponse) {
  ASSERT_MAIN_THREAD()
  trace("HttpRequest::_schedule_on_headers_complete_complete");

  boost::function<void (void)> cb(
    boost::bind(&HttpRequest::_on_headers_complete_complete, this, pResponse)
  );
  _background_queue->push(cb);

  if (pResponse)
    responseScheduled();
}

// This is called after the user's R onHeaders() function has finished. It can
// write a response, if onHeaders() wants that. It also sets a status code for
// http-parser and then re-executes the parser. Runs on the background thread.
void HttpRequest::_on_headers_complete_complete(HttpResponse* pResponse) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_headers_complete_complete");

  if (!_decrement_reference()) {
    return;
  }

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
// Message body (for POST)
// ============================================================================

int HttpRequest::_on_body(http_parser* pParser, const char* pAt, size_t length) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_body");

  // Copy pAt because the source data is deleted right after calling this
  // function.
  std::vector<char>* buf = new std::vector<char>(pAt, pAt + length);

  boost::function<void(HttpResponse*)> schedule_bg_callback(
    boost::bind(&HttpRequest::_schedule_on_body_error, this, _1)
  );

  // Schedule on main thread:
  // _pWebApplication->onBodyData(this, pAt, length);
  BoostFunctionCallback* webapp_on_body_data_callback = new BoostFunctionCallback(
    boost::bind(
      &WebApplication::onBodyData,
      _pWebApplication,
      this,
      &(*buf)[0],
      length,
      schedule_bg_callback
    )
  );
  later::later(invoke_callback, webapp_on_body_data_callback, 0);

  // Schedule for after on_ws_message_callback:
  // delete_cb_main<std::vector<char>*>(buf)
  later::later(delete_cb_main<std::vector<char>*>, buf, 0);

  // TODO: _bytesRead currently is not used anywhere else. If this changes,
  // then depending on how it's used, it might make more sense to have
  // _pWebApplication->onBodyData() set this, or schedule a callback that sets
  // it on the background thread.
  _bytesRead += length;

  return 0;
}

void HttpRequest::_schedule_on_body_error(HttpResponse* pResponse) {
  ASSERT_MAIN_THREAD()
  trace("HttpRequest::_schedule_on_body_error");

  boost::function<void (void)> cb(
    boost::bind(&HttpRequest::_on_body_error, this, pResponse)
  );
  _background_queue->push(cb);

  responseScheduled();
}

void HttpRequest::_on_body_error(HttpResponse* pResponse) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_body_error");

  http_parser_pause(&_parser, 1);

  pResponse->closeAfterWritten();
  uv_read_stop((uv_stream_t*)handle());
  _ignoreNewData = true;

  pResponse->writeResponse();
}


// ============================================================================
// Message complete
// ============================================================================

int HttpRequest::_on_message_complete(http_parser* pParser) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_message_complete");

  if (pParser->upgrade)
    return 0;

  boost::function<void(HttpResponse*)> schedule_bg_callback(
    boost::bind(&HttpRequest::_schedule_on_message_complete_complete, this, _1)
  );

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

  _increment_reference();

  return 0;
}

// This is called by the user's application code during or after the end of
// WebApplication::getResponse(). It puts an item on the background queue.
void HttpRequest::_schedule_on_message_complete_complete(HttpResponse* pResponse) {
  ASSERT_MAIN_THREAD()

  boost::function<void (void)> cb(
    boost::bind(&HttpRequest::_on_message_complete_complete, this, pResponse)
  );
  _background_queue->push(cb);

  responseScheduled();
}

void HttpRequest::_on_message_complete_complete(HttpResponse* pResponse) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_message_complete_complete");

  if (!_decrement_reference()) {
    return;
  }

  // This can happen if an error occured in WebApplication::onBodyData.
  if (pResponse == NULL) {
    return;
  }

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

// Called from WebSocketConnection::onFrameComplete
void HttpRequest::onWSMessage(bool binary, const char* data, size_t len) {
  ASSERT_BACKGROUND_THREAD()

  // Copy data because the source data is deleted right after calling this
  // function.
  std::vector<char>* buf = new std::vector<char>(data, data + len);

  boost::function<void (void)> error_callback(
    boost::bind(&HttpRequest::schedule_close, this)
  );

  // Schedule:
  // _pWebApplication->onWSMessage(_pWebSocketConnection, binary, data, len);
  BoostFunctionCallback* on_ws_message_callback = new BoostFunctionCallback(
    boost::bind(
      &WebApplication::onWSMessage,
      _pWebApplication,
      _pWebSocketConnection,
      binary,
      &(*buf)[0],
      len,
      error_callback
    )
  );
  later::later(invoke_callback, on_ws_message_callback, 0);

  // Schedule for after on_ws_message_callback:
  // delete_cb_main<std::vector<char>*>(buf)
  later::later(delete_cb_main<std::vector<char>*>, buf, 0);
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

// Because these functions always run in the same thread, there's no need for
// locking.
// TODO: Replace these constructs with something simpler and more robust
void HttpRequest::_increment_reference() {
  ASSERT_BACKGROUND_THREAD()
  _ref_count++;
  trace("HttpRequest::_increment_reference:" + std::to_string(_ref_count));
}

bool HttpRequest::_decrement_reference() {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_decrement_reference:" + std::to_string(_ref_count - 1));

  if (--_ref_count == 0) {
    trace("deleting HttpRequest object");

    delete this;
    return false;
  }

  return true;
}

void HttpRequest::_on_closed(uv_handle_t* handle) {
  trace("HttpRequest::_on_closed");
  _decrement_reference();
}

void HttpRequest::close() {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::close");
  // std::cerr << "Closing handle " << &_handle << std::endl;

  if (_is_closing) {
    trace("close() called twice on HttpRequest object");
    // Shouldn't get here, but just in case close() gets called twice
    // (probably via a scheduled callback), don't do the closing machinery
    // twice.
    _on_closed(NULL);
    return;
  }
  _is_closing = true;

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

// This is to be called from the main thread, when the main thread needs to
// tell the background thread to close the request. The main thread should not
// close() directly.
void HttpRequest::schedule_close() {
  // Schedule on background thread:
  //  pRequest->close()
  _background_queue->push(
    boost::bind(&HttpRequest::close, this)
  );
}


// ============================================================================
// Open websocket
// ============================================================================

void HttpRequest::_call_r_on_ws_open() {
  ASSERT_MAIN_THREAD()
  trace("_call_r_on_ws_open");

  boost::function<void (void)> error_callback(
    boost::bind(&HttpRequest::schedule_close, this)
  );
  this->_pWebApplication->onWSOpen(this, error_callback);

  // _requestBuffer is likely empty at this point, but copy its contents and
  // _pass along just in case.
  std::vector<char>* req_buffer = new std::vector<char>(_requestBuffer);
  _requestBuffer.clear();

  // Schedule on background thread:
  // _pWebSocketConnection->read(&(*req_buffer)[0], req_buffer->size())
  boost::function<void (void)> cb(
    boost::bind(&WebSocketConnection::read,
      _pWebSocketConnection,
      &(*req_buffer)[0],
      req_buffer->size()
    )
  );

  _background_queue->push(cb);
  // Free req_buffer after data is written
  _background_queue->push(boost::bind(delete_cb_bg<std::vector<char>*>, req_buffer));
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

      _requestBuffer.insert(_requestBuffer.end(), pData, pData + pDataLen);

      // Schedule on main thread:
      // this->_call_r_on_ws_open()
      BoostFunctionCallback* call_r_on_ws_open_callback = new BoostFunctionCallback(
        boost::bind(&HttpRequest::_call_r_on_ws_open, this)
      );
      later::later(invoke_callback, (void*)call_r_on_ws_open_callback, 0);
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
