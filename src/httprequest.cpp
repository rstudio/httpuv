#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <later_api.h>
#include "httprequest.h"
#include "callback.h"
#include "utils.h"
#include "thread.h"
#include "auto_deleter.h"


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

// Each HttpRequest object represents a connection. Multiple actual HTTP
// requests can happen in sequence on this connection. Each time a new message
// starts, we need to reset some parts of the HttpRequest object.
void HttpRequest::_newRequest() {
  ASSERT_BACKGROUND_THREAD()

  if (_handling_request) {
    err_printf("Error: pipelined HTTP requests not supported.\n");
    close();
  }

  _handling_request = true;
  _headers.clear();
  _response_scheduled = false;

  // Schedule on main thread:
  //   this->_initializeEnv();
  invoke_later(
    boost::bind(&HttpRequest::_initializeEnv, shared_from_this())
  );
}

void HttpRequest::_initializeEnv() {
  ASSERT_MAIN_THREAD()
  using namespace Rcpp;

  Environment base(R_BaseEnv);
  Function new_env = Rcpp::as<Function>(base["new.env"]);

  // The deleter is called either when this function is called again, or when
  // the HttpRequest object is deleted. The deletion will happen on the
  // background thread; auto_deleter_main() schedules the deletion of the
  // Rcpp::Environment object on the main thread.
  _env = boost::shared_ptr<Environment>(
    new Environment(new_env(_["parent"] = R_EmptyEnv)),
    auto_deleter_main<Environment>
  );
}

Rcpp::Environment& HttpRequest::env() {
  ASSERT_MAIN_THREAD()
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

void HttpRequest::requestCompleted() {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::requestCompleted");
  _handling_request = false;
}


// ============================================================================
// Miscellaneous callbacks for http parser
// ============================================================================

int HttpRequest::_on_message_begin(http_parser* pParser) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_message_begin");
  _newRequest();
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

  boost::function<void(boost::shared_ptr<HttpResponse>)> schedule_bg_callback(
    boost::bind(&HttpRequest::_schedule_on_headers_complete_complete, shared_from_this(), _1)
  );

  // Use later to schedule _pWebApplication->onHeaders(this, schedule_bg_callback)
  // to run on the main thread. That function in turn calls
  // this->_schedule_on_headers_complete_complete.
  invoke_later(
    boost::bind(
      &WebApplication::onHeaders,
      _pWebApplication,
      shared_from_this(),
      schedule_bg_callback
    )
  );

  return 0;
}

// This is called at the end of WebApplication::onHeaders(). It puts an item
// on the write queue and signals to the background thread that there's
// something there.
void HttpRequest::_schedule_on_headers_complete_complete(boost::shared_ptr<HttpResponse> pResponse) {
  ASSERT_MAIN_THREAD()
  trace("HttpRequest::_schedule_on_headers_complete_complete");

  if (pResponse)
    responseScheduled();

  boost::function<void (void)> cb(
    boost::bind(&HttpRequest::_on_headers_complete_complete, shared_from_this(), pResponse)
  );
  _background_queue->push(cb);
}

// This is called after the user's R onHeaders() function has finished. It can
// write a response, if onHeaders() wants that. It also sets a status code for
// http-parser and then re-executes the parser. Runs on the background thread.
void HttpRequest::_on_headers_complete_complete(boost::shared_ptr<HttpResponse> pResponse) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_headers_complete_complete");

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
      pResponse = boost::shared_ptr<HttpResponse>(
        new HttpResponse(shared_from_this(), 100, "Continue", (DataSource*)NULL),
        auto_deleter_background<HttpResponse>
      );
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
  boost::shared_ptr<std::vector<char> > buf = boost::make_shared<std::vector<char> >(pAt, pAt + length);

  boost::function<void(boost::shared_ptr<HttpResponse>)> schedule_bg_callback(
    boost::bind(&HttpRequest::_schedule_on_body_error, shared_from_this(), _1)
  );

  // Schedule on main thread:
  // _pWebApplication->onBodyData(this, pAt, length, schedule_bg_callback);
  invoke_later(
    boost::bind(
      &WebApplication::onBodyData,
      _pWebApplication,
      shared_from_this(),
      buf,
      schedule_bg_callback
    )
  );

  return 0;
}

void HttpRequest::_schedule_on_body_error(boost::shared_ptr<HttpResponse> pResponse) {
  ASSERT_MAIN_THREAD()
  trace("HttpRequest::_schedule_on_body_error");

  responseScheduled();

  boost::function<void (void)> cb(
    boost::bind(&HttpRequest::_on_body_error, shared_from_this(), pResponse)
  );
  _background_queue->push(cb);

}

void HttpRequest::_on_body_error(boost::shared_ptr<HttpResponse> pResponse) {
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

  boost::function<void(boost::shared_ptr<HttpResponse>)> schedule_bg_callback(
    boost::bind(&HttpRequest::_schedule_on_message_complete_complete, shared_from_this(), _1)
  );

  // Use later to schedule _pWebApplication->getResponse(this, schedule_bg_callback)
  // to run on the main thread. That function in turn calls
  // this->_schedule_on_message_complete_complete.
  invoke_later(
    boost::bind(
      &WebApplication::getResponse,
      _pWebApplication,
      shared_from_this(),
      schedule_bg_callback
    )
  );

  return 0;
}

// This is called by the user's application code during or after the end of
// WebApplication::getResponse(). It puts an item on the background queue.
void HttpRequest::_schedule_on_message_complete_complete(boost::shared_ptr<HttpResponse> pResponse) {
  ASSERT_MAIN_THREAD()

  responseScheduled();

  boost::function<void (void)> cb(
    boost::bind(&HttpRequest::_on_message_complete_complete, shared_from_this(), pResponse)
  );
  _background_queue->push(cb);
}

void HttpRequest::_on_message_complete_complete(boost::shared_ptr<HttpResponse> pResponse) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_message_complete_complete");

  // This can happen if an error occured in WebApplication::onBodyData.
  if (pResponse == NULL) {
    return;
  }

  // TODO: ADding this fixes the ERROR: [uv_write] bad file descriptor, but
  // then we need to make sure the pResponse gets cleaned up. Smart pointer?
  if (_is_closing)
    return;

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
  trace("HttpRequest::onWSMessage");

  // Copy data because the source data is deleted right after calling this
  // function.
  boost::shared_ptr<std::vector<char> > buf = boost::make_shared<std::vector<char> >(data, data + len);

  boost::function<void (void)> error_callback(
    boost::bind(&HttpRequest::schedule_close, shared_from_this())
  );

  boost::shared_ptr<WebSocketConnection> p_wsc = _pWebSocketConnection;
  // It's possible for _pWebSocketConnection to have had its refcount drop to
  // zero from another thread or earlier callback in this thread. If that
  // happened, do nothing.
  if (!p_wsc) {
    return;
  }

  // Schedule:
  // _pWebApplication->onWSMessage(p_wsc, binary, data, len);
  invoke_later(
    boost::bind(
      &WebApplication::onWSMessage,
      _pWebApplication,
      p_wsc,
      binary,
      buf,
      error_callback
    )
  );
}

void HttpRequest::onWSClose(int code) {
  trace("HttpRequest::onWSClose");
  // TODO: Call close() here?
}

void HttpRequest::fatal_error(const char* method, const char* message) {
  err_printf("ERROR: [%s] %s\n", method, message);
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
  trace("on_ws_message_sent");
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
  trace("HttpRequest::sendWSFrame");
  ws_send_t* pSend = (ws_send_t*)malloc(sizeof(ws_send_t));
  memset(pSend, 0, sizeof(ws_send_t));
  pSend->pHeader = new std::vector<char>(pHeader, pHeader + headerSize);
  pSend->pData = new std::vector<char>(pData, pData + dataSize);
  pSend->pFooter = new std::vector<char>(pFooter, pFooter + footerSize);

  uv_buf_t buffers[3];
  buffers[0] = uv_buf_init(safe_vec_addr(*pSend->pHeader), pSend->pHeader->size());
  buffers[1] = uv_buf_init(safe_vec_addr(*pSend->pData), pSend->pData->size());
  buffers[2] = uv_buf_init(safe_vec_addr(*pSend->pFooter), pSend->pFooter->size());

  // TODO: Handle return code
  uv_write(&pSend->writeReq, (uv_stream_t*)handle(), buffers, 3,
           &on_ws_message_sent);
}

void HttpRequest::closeWSSocket() {
  trace("HttpRequest::closeWSSocket");
  close();
}


// ============================================================================
// Closing connection
// ============================================================================

void HttpRequest::_on_closed(uv_handle_t* handle) {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::_on_closed");

  boost::shared_ptr<WebSocketConnection> p_wsc = _pWebSocketConnection;
  // It's possible for _pWebSocketConnection to have had its refcount drop to
  // zero from another thread or earlier callback in this thread. If that
  // happened, do nothing.
  if (!p_wsc) {
    return;
  }

  // Tell the WebSocketConnection that the connection is closed, before
  // resetting the shared_ptr. This is useful because there may be some
  // callbacks that will execute later, and we want to make sure the WSC
  // doesn't try to do anything with them.
  p_wsc->markClosed();

  // Note that this location and the destructor are the only places where
  // _pWebSocketConnection is reset; both are on the background thread.
  _pWebSocketConnection.reset();
}

void HttpRequest::close() {
  ASSERT_BACKGROUND_THREAD()
  trace("HttpRequest::close");
  // std::cerr << "Closing handle " << &_handle << std::endl;

  if (_is_closing) {
    trace("close() called twice on HttpRequest object");
    // We can get here in unusual cases when close() is called once directly,
    // and another time via a scheduled callback. When this happens, don't do
    // the closing machinery twice.
    return;
  }
  _is_closing = true;

  boost::shared_ptr<WebSocketConnection> p_wsc = _pWebSocketConnection;

  if (p_wsc && _protocol == WebSockets) {
    // Schedule:
    // _pWebApplication->onWSClose(p_wsc)
    invoke_later(
      boost::bind(
        &WebApplication::onWSClose,
        _pWebApplication,
        p_wsc
      )
    );
  }

  _pSocket->removeConnection(shared_from_this());

  uv_close(toHandle(&_handle.stream), HttpRequest_on_closed);
}

// This is to be called from the main thread, when the main thread needs to
// tell the background thread to close the request. The main thread should not
// close() directly.
void HttpRequest::schedule_close() {
  trace("HttpRequest::schedule_close");
  // Schedule on background thread:
  //  pRequest->close()
  _background_queue->push(
    boost::bind(&HttpRequest::close, shared_from_this())
  );
}


// ============================================================================
// Open websocket
// ============================================================================

void HttpRequest::_call_r_on_ws_open() {
  ASSERT_MAIN_THREAD()
  trace("_call_r_on_ws_open");

  boost::function<void (void)> error_callback(
    boost::bind(&HttpRequest::schedule_close, shared_from_this())
  );

  this->_pWebApplication->onWSOpen(shared_from_this(), error_callback);

  boost::shared_ptr<WebSocketConnection> p_wsc = _pWebSocketConnection;
  // It's possible for _pWebSocketConnection to have had its refcount drop to
  // zero from another thread or earlier callback in this thread. If that
  // happened, do nothing.
  if (!p_wsc) {
    return;
  }

  // _requestBuffer is likely empty at this point, but copy its contents and
  // _pass along just in case.

  boost::shared_ptr<std::vector<char> > req_buffer = boost::make_shared<std::vector<char> >(_requestBuffer);
  _requestBuffer.clear();


  // Schedule on background thread:
  // p_wsc->read(safe_vec_addr(*req_buffer), req_buffer->size())
  boost::function<void (void)> cb(
    boost::bind(&WebSocketConnection::read,
      p_wsc,
      safe_vec_addr(*req_buffer),
      req_buffer->size()
    )
  );

  _background_queue->push(cb);
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

    boost::shared_ptr<WebSocketConnection> p_wsc = _pWebSocketConnection;
    // It's possible for _pWebSocketConnection to have had its refcount drop to
    // zero from another thread or earlier callback in this thread. If that
    // happened, do nothing.
    if (!p_wsc) {
      return;
    }

    if (p_wsc->accept(_headers, pData, pDataLen)) {
      // Freed in on_response_written
      InMemoryDataSource* pDS = new InMemoryDataSource();
      boost::shared_ptr<HttpResponse> pResp(
        new HttpResponse(shared_from_this(), 101, "Switching Protocols", pDS),
        auto_deleter_background<HttpResponse>
      );

      std::vector<uint8_t> body;
      p_wsc->handshake(_url, _headers, &pData, &pDataLen,
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
      invoke_later(
        boost::bind(&HttpRequest::_call_r_on_ws_open, shared_from_this())
      );
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

  this->_parse_http_data(safe_vec_addr(req_buffer), req_buffer.size());
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
      boost::shared_ptr<WebSocketConnection> p_wsc = _pWebSocketConnection;
      // It's possible for _pWebSocketConnection to have had its refcount drop to
      // zero from another thread or earlier callback in this thread. If that
      // happened, do nothing.
      if (p_wsc) {
        p_wsc->read(buf->base, nread);
      }
    }
  } else if (nread < 0) {
    if (nread == UV_EOF || nread == UV_ECONNRESET) {
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
