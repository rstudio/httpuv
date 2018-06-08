#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <map>
#include <iostream>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include "libuv/include/uv.h"
#include "http-parser/http_parser.h"
#include "socket.h"
#include "webapplication.h"
#include "callbackqueue.h"
#include "httpresponse.h"
#include "utils.h"
#include "thread.h"
#include "auto_deleter.h"

enum Protocol {
  HTTP,
  WebSockets
};

// HttpRequest is a bit of a misnomer -- a HttpRequest object represents a
// single connection, on which multiple actual HTTP requests can be made.
class HttpRequest : public WebSocketConnectionCallbacks,
                    public boost::enable_shared_from_this<HttpRequest> 
{
private:
  uv_loop_t* _pLoop;
  boost::shared_ptr<WebApplication> _pWebApplication;
  VariantHandle _handle;
  boost::shared_ptr<Socket> _pSocket;
  http_parser _parser;
  Protocol _protocol;
  std::string _url;
  RequestHeaders _headers;
  std::string _lastHeaderField;
  boost::shared_ptr<WebSocketConnection> _pWebSocketConnection;

  // `_env` is an shared_ptr<Environment> instead of an Environment because it
  // must be created and deleted on the main thread. However, the creation and
  // deletion of HttpRequest objects happens on the background thread, and so
  // the lifetime of the Environment can't be strictly tied to the lifetime of
  // the HttpRequest. It is instantiated with a deleter function that ensures
  // deletion happens on the main thread.
  boost::shared_ptr<Rcpp::Environment> _env;
  void _newRequest();
  void _initializeEnv();

  // _ignoreNewData is used in cases where we rejected a request (by sending
  // a response with a non-100 status code) before its body was received. We
  // don't want to close the connection because the response might not be
  // sent yet, but we don't want to parse any more data from this connection.
  // (You would think uv_stop_read could be called, but it seems to prevent
  // the response from being written as well.)
  bool _ignoreNewData;

  bool _is_closing;

  bool _hasHeader(const std::string& name) const;
  bool _hasHeader(const std::string& name, const std::string& value, bool ci = false) const;

  void _parse_http_data(char* buf, const ssize_t n);
  // Parse data that has been stored in the buffer.
  void _parse_http_data_from_buffer();

  bool _response_scheduled;
  // True when the HttpRequest object is handling an HTTP request; gets set to
  // false when the response is written.
  bool _handling_request;

  // For buffering the incoming HTTP request when data comes in while waiting
  // for R to process headers.
  std::vector<char> _requestBuffer;

  // Most of the methods in HttpRequest run on a background thread. Some
  // methods run on the main thread. This is used by the main-thread methods
  // to schedule callbacks to run on the background thread.
  CallbackQueue* _background_queue;

public:
  HttpRequest(uv_loop_t* pLoop,
              boost::shared_ptr<WebApplication> pWebApplication,
              boost::shared_ptr<Socket> pSocket,
              CallbackQueue* backgroundQueue)
    : _pLoop(pLoop),
      _pWebApplication(pWebApplication),
      _pSocket(pSocket),
      _protocol(HTTP),
      _ignoreNewData(false),
      _is_closing(false),
      _response_scheduled(false),
      _handling_request(false),
      _background_queue(backgroundQueue)
  {
    ASSERT_BACKGROUND_THREAD()
    uv_tcp_init(pLoop, &_handle.tcp);
    _handle.isTcp = true;
    // This is used by the macro-defined callbacks like _on_request_read
    _handle.stream.data = this;

    http_parser_init(&_parser, HTTP_REQUEST);
    // This is used by the macro-defined callbacks like _on_message_begin
    _parser.data = this;
  }

  virtual ~HttpRequest() {
    ASSERT_BACKGROUND_THREAD()
    trace("HttpRequest::~HttpRequest");
    _pWebSocketConnection.reset();
  }

  uv_stream_t* handle();
  boost::shared_ptr<WebSocketConnection> websocket() const {
    return _pWebSocketConnection;
  }
  Address clientAddress();
  Address serverAddress();
  Rcpp::Environment& env();

  void handleRequest();

  std::string method() const;
  std::string url() const;
  const RequestHeaders& headers() const;
  // Is the request an Upgrade (i.e. WebSocket connection)?
  bool isUpgrade() const;
  
  void sendWSFrame(const char* pHeader, size_t headerSize,
                   const char* pData, size_t dataSize,
                   const char* pFooter, size_t footerSize);
  void closeWSSocket();

  // Call this function from the main thread to indicate that a response has
  // been scheduled. This is needed because sometimes by the time the main
  // thread knows that it needs to send a response, the bg thread will have
  // kept going and scheduled another call into the main thread to send a
  // response.
  void responseScheduled();
  bool isResponseScheduled();

  // This function should be called when a single request has been completed
  // (when the response has been sent). It is currently used to detect
  // pipelined HTTP requests.
  void requestCompleted();

  void _call_r_on_ws_open();
  void _schedule_on_headers_complete_complete(boost::shared_ptr<HttpResponse> pResponse);
  void _on_headers_complete_complete(boost::shared_ptr<HttpResponse> pResponse);
  void _schedule_on_body_error(boost::shared_ptr<HttpResponse> pResponse);
  void _on_body_error(boost::shared_ptr<HttpResponse> pResponse);
  void _schedule_on_message_complete_complete(boost::shared_ptr<HttpResponse> pResponse);
  void _on_message_complete_complete(boost::shared_ptr<HttpResponse> pResponse);

public:
  // Callbacks
  virtual int _on_message_begin(http_parser* pParser);
  virtual int _on_url(http_parser* pParser, const char* pAt, size_t length);
  virtual int _on_status(http_parser* pParser, const char* pAt, size_t length);
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
  void schedule_close();
  void _on_request_read(uv_stream_t*, ssize_t nread, const uv_buf_t* buf);
  void _on_response_write(int status);

  void _initializeSocket() {
    // Coerce to parent class
    boost::shared_ptr<WebSocketConnectionCallbacks> this_base(
      boost::static_pointer_cast<WebSocketConnectionCallbacks>(shared_from_this())
    );

    _pWebSocketConnection = boost::shared_ptr<WebSocketConnection>(
      new WebSocketConnection(this_base),
      auto_deleter_background<WebSocketConnection>
    );

    _pSocket->addConnection(shared_from_this());
  }
};


// Same for Websocketconnection
// Factory function needed because we can't call shared_from_this() inside the
// constructor.
inline boost::shared_ptr<HttpRequest> createHttpRequest(
  uv_loop_t* pLoop,
  boost::shared_ptr<WebApplication> pWebApplication,
  boost::shared_ptr<Socket> pSocket,
  CallbackQueue* backgroundQueue)
{
  ASSERT_BACKGROUND_THREAD()

  // The shared_ptr has a custom deleter which ensures that the HttpRequest is
  // deleted on the background thread.
  boost::shared_ptr<HttpRequest> req(
    new HttpRequest(pLoop, pWebApplication, pSocket, backgroundQueue),
    auto_deleter_background<HttpRequest>
  );

  req->_initializeSocket();

  return req;
}


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
