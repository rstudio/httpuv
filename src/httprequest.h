#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <map>
#include <iostream>

#include <boost/function.hpp>

#include <uv.h>
#include <http_parser.h>
#include "socket.h"
#include "webapplication.h"
#include "httpresponse.h"


enum Protocol {
  HTTP,
  WebSockets
};


class HttpRequest : WebSocketConnectionCallbacks {

private:
  uv_loop_t* _pLoop;
  WebApplication* _pWebApplication;
  VariantHandle _handle;
  Socket* _pSocket;
  http_parser _parser;
  Protocol _protocol;
  std::string _url;
  RequestHeaders _headers;
  std::string _lastHeaderField;
  unsigned long _bytesRead;
  Rcpp::Environment _env;
  WebSocketConnection* _pWebSocketConnection;
  // _ignoreNewData is used in cases where we rejected a request (by sending
  // a response with a non-100 status code) before its body was received. We
  // don't want to close the connection because the response might not be
  // sent yet, but we don't want to parse any more data from this connection.
  // (You would think uv_stop_read could be called, but it seems to prevent
  // the response from being written as well.)
  bool _ignoreNewData;

  void trace(const std::string& msg);

  bool _hasHeader(const std::string& name) const;
  bool _hasHeader(const std::string& name, const std::string& value, bool ci = false) const;

  void _parse_http_data(char* buf, const ssize_t n);
  // Parse data that has been stored in the buffer.
  void _parse_http_data_from_buffer();

  // For buffering the incoming HTTP request when data comes in while waiting
  // for R to process headers.
  std::vector<char> _requestBuffer;

public:
  HttpRequest(uv_loop_t* pLoop, WebApplication* pWebApplication,
      Socket* pSocket)
    : _pLoop(pLoop), _pWebApplication(pWebApplication), _pSocket(pSocket),
      _protocol(HTTP), _bytesRead(0),
      _pWebSocketConnection(new WebSocketConnection(this)),
      _ignoreNewData(false) {

    uv_tcp_init(pLoop, &_handle.tcp);
    _handle.isTcp = true;
    _handle.stream.data = this;

    http_parser_init(&_parser, HTTP_REQUEST);
    _parser.data = this;

    _pSocket->addConnection(this);

    _env = Rcpp::Function("new.env")();
  }

  virtual ~HttpRequest() {
    try {
      delete _pWebSocketConnection;
    } catch (...) {}
  }

  uv_stream_t* handle();
  WebSocketConnection* websocket() const { return _pWebSocketConnection; }
  Address clientAddress();
  Address serverAddress();
  Rcpp::Environment& env();

  void handleRequest();

  std::string method() const;
  std::string url() const;
  const RequestHeaders& headers() const;

  void sendWSFrame(const char* pHeader, size_t headerSize,
                   const char* pData, size_t dataSize,
                   const char* pFooter, size_t footerSize);
  void closeWSSocket();

public:
  // Callbacks
  virtual int _on_message_begin(http_parser* pParser);
  virtual int _on_url(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_status(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_header_field(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_header_value(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_headers_complete(http_parser* pParser);
  virtual void _call_r_on_headers();
  virtual void _on_headers_complete_complete(HttpResponse* pResponse);
  virtual int _on_body(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_message_complete(http_parser* pParser);
  virtual void _on_message_complete_complete(HttpResponse* pResponse);

  virtual void onWSMessage(bool binary, const char* data, size_t len);
  virtual void onWSClose(int code);

  void fatal_error(const char* method, const char* message);
  void _on_closed(uv_handle_t* handle);
  void close();
  void _on_request_read(uv_stream_t*, ssize_t nread, const uv_buf_t* buf);
  void _on_response_write(int status);

};


#define DECLARE_CALLBACK_1(type, function_name, return_type, type_1) \
  return_type type##_##function_name(type_1 arg1);
#define DECLARE_CALLBACK_3(type, function_name, return_type, type_1, type_2, type_3) \
  return_type type##_##function_name(type_1 arg1, type_2 arg2, type_3 arg3);
#define DECLARE_CALLBACK_2(type, function_name, return_type, type_1, type_2) \
  return_type type##_##function_name(type_1 arg1, type_2 arg2);

DECLARE_CALLBACK_1(HttpRequest, on_message_begin, int, http_parser*)
DECLARE_CALLBACK_3(HttpRequest, on_url, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_3(HttpRequest, on_status, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_3(HttpRequest, on_header_field, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_3(HttpRequest, on_header_value, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(HttpRequest, on_headers_complete, int, http_parser*)
DECLARE_CALLBACK_3(HttpRequest, on_body, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(HttpRequest, on_message_complete, int, http_parser*)
DECLARE_CALLBACK_1(HttpRequest, on_closed, void, uv_handle_t*)
DECLARE_CALLBACK_3(HttpRequest, on_request_read, void, uv_stream_t*, ssize_t, const uv_buf_t*)
DECLARE_CALLBACK_2(HttpRequest, on_response_write, void, uv_write_t*, int)

#endif // HTTPREQUEST_HPP
