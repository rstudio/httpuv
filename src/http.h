#ifndef HTTP_HPP
#define HTTP_HPP

#include <map>
#include <string>
#include <iostream>
#include <vector>

#include <Rcpp.h>
#undef Realloc
// Also need to undefine the Free macro
#undef Free
#include <uv.h>
#include <http_parser.h>

#include "websockets.h"
#include "uvutil.h"

struct compare_ci {
  bool operator()(const std::string& a, const std::string& b) const {
    return strcasecmp(a.c_str(), b.c_str()) < 0;
  }
};

class HttpRequest;
class HttpResponse;

enum Protocol {
  HTTP,
  WebSockets
};

class WebApplication {
public:
  virtual ~WebApplication() {}
  virtual HttpResponse* onHeaders(HttpRequest* pRequest) {
    return NULL;
  }
  virtual void onBodyData(HttpRequest* pRequest,
                          const char* data, size_t len) = 0;
  virtual HttpResponse* getResponse(HttpRequest* request) = 0;
  virtual void onWSOpen(HttpRequest* pRequest) = 0;
  virtual void onWSMessage(WebSocketConnection* conn,
                           bool binary, const char* data, size_t len) = 0;
  virtual void onWSClose(WebSocketConnection* conn) = 0;
};

typedef struct {
  union {
    uv_stream_t stream;
    uv_tcp_t tcp;
    uv_pipe_t pipe;
  };
  bool isTcp;
} VariantHandle;

class Socket {
public:
  VariantHandle handle;
  WebApplication* pWebApplication;
  std::vector<HttpRequest*> connections;

  Socket() {
  }

  void addConnection(HttpRequest* request);
  void removeConnection(HttpRequest* request);

  virtual ~Socket();
  virtual void destroy();
};

struct Address {
  std::string host;
  unsigned short port;

  Address() : port(0) {
  }
};

class HttpRequest : WebSocketConnection {

private:
  uv_loop_t* _pLoop;
  WebApplication* _pWebApplication;
  VariantHandle _handle;
  Socket* _pSocket;
  http_parser _parser;
  Protocol _protocol;
  std::string _url;
  std::map<std::string, std::string, compare_ci> _headers;
  std::string _lastHeaderField;
  unsigned long _bytesRead;
  Rcpp::Environment _env;
  // _ignoreNewData is used in cases where we rejected a request (by sending
  // a response with a non-100 status code) before its body was received. We
  // don't want to close the connection because the response might not be
  // sent yet, but we don't want to parse any more data from this connection.
  // (You would think uv_stop_read could be called, but it seems to prevent
  // the response from being written as well.)
  bool _ignoreNewData;

  void trace(const std::string& msg);

public:
  HttpRequest(uv_loop_t* pLoop, WebApplication* pWebApplication,
      Socket* pSocket)
    : _pLoop(pLoop), _pWebApplication(pWebApplication), _pSocket(pSocket),
      _protocol(HTTP), _bytesRead(0), _ignoreNewData(false) {

    uv_tcp_init(pLoop, &_handle.tcp);
    _handle.isTcp = true;
    _handle.stream.data = this;

    http_parser_init(&_parser, HTTP_REQUEST);
    _parser.data = this;

    _pSocket->addConnection(this);
    
    _env = Rcpp::Function("new.env")();
  }

  virtual ~HttpRequest() {
  }

  uv_stream_t* handle();
  Address serverAddress();
  Rcpp::Environment& env();

  void handleRequest();

  std::string method() const;
  std::string url() const;
  std::map<std::string, std::string, compare_ci> headers() const;

  void sendWSFrame(const char* pHeader, size_t headerSize,
                   const char* pData, size_t dataSize);
  void closeWSSocket();

public:
  // Callbacks
  virtual int _on_message_begin(http_parser* pParser);
  virtual int _on_url(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_status_complete(http_parser* pParser);
  virtual int _on_header_field(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_header_value(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_headers_complete(http_parser* pParser);
  virtual int _on_body(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_message_complete(http_parser* pParser);

  virtual void onWSMessage(bool binary, const char* data, size_t len);
  virtual void onWSClose(int code);

  void fatal_error(const char* method, const char* message);
  void _on_closed(uv_handle_t* handle);
  void close();
  void _on_request_read(uv_stream_t*, ssize_t nread, uv_buf_t buf);
  void _on_response_write(int status);

};

struct HttpResponse {

  HttpRequest* _pRequest;
  int _statusCode;
  std::string _status;
  std::vector<std::pair<std::string, std::string> > _headers;
  std::vector<char> _responseHeader;
  DataSource* _pBody;

public:
  HttpResponse(HttpRequest* pRequest, int statusCode,
         const std::string& status, DataSource* pBody)
    : _pRequest(pRequest), _statusCode(statusCode), _status(status), _pBody(pBody) {

  }

  virtual ~HttpResponse() {
  }

  void addHeader(const std::string& name, const std::string& value);
  void writeResponse();
  void onResponseWritten(int status);
};

#define DECLARE_CALLBACK_1(type, function_name, return_type, type_1) \
  return_type type##_##function_name(type_1 arg1);
#define DECLARE_CALLBACK_3(type, function_name, return_type, type_1, type_2, type_3) \
  return_type type##_##function_name(type_1 arg1, type_2 arg2, type_3 arg3);
#define DECLARE_CALLBACK_2(type, function_name, return_type, type_1, type_2) \
  return_type type##_##function_name(type_1 arg1, type_2 arg2);

DECLARE_CALLBACK_1(HttpRequest, on_message_begin, int, http_parser*)
DECLARE_CALLBACK_3(HttpRequest, on_url, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(HttpRequest, on_status_complete, int, http_parser*)
DECLARE_CALLBACK_3(HttpRequest, on_header_field, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_3(HttpRequest, on_header_value, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(HttpRequest, on_headers_complete, int, http_parser*)
DECLARE_CALLBACK_3(HttpRequest, on_body, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(HttpRequest, on_message_complete, int, http_parser*)
DECLARE_CALLBACK_1(HttpRequest, on_closed, void, uv_handle_t*)
DECLARE_CALLBACK_3(HttpRequest, on_request_read, void, uv_stream_t*, ssize_t, uv_buf_t)
DECLARE_CALLBACK_2(HttpRequest, on_response_write, void, uv_write_t*, int)

uv_stream_t* createPipeServer(uv_loop_t* loop, const std::string& name,
  int mask, WebApplication* pWebApplication);
uv_stream_t* createTcpServer(uv_loop_t* loop, const std::string& host, int port,
  WebApplication* pWebApplication);
void freeServer(uv_stream_t* pServer);
bool runNonBlocking(uv_loop_t* loop);

#endif // HTTP_HPP
