#ifndef WEBAPPLICATION_HPP
#define WEBAPPLICATION_HPP

#include <boost/function.hpp>
#include <uv.h>
#include <Rcpp.h>
#include "websockets.h"
#include "debug.h"

class HttpRequest;
class HttpResponse;

class WebApplication {
public:
  virtual ~WebApplication() {}
  virtual void onHeaders(HttpRequest* pRequest, boost::function<void(HttpResponse*)> callback) = 0;
  virtual void onBodyData(HttpRequest* pRequest,
                          const char* data, size_t len,
                          boost::function<void(HttpResponse*)> errorCallback) = 0;
  virtual void getResponse(HttpRequest* request, boost::function<void(HttpResponse*)> callback) = 0;
  virtual void onWSOpen(HttpRequest* pRequest, boost::function<void(void)> error_callback) = 0;
  virtual void onWSMessage(WebSocketConnection* conn,
                           bool binary, const char* data, size_t len,
                           boost::function<void(void)> error_callback) = 0;
  virtual void onWSClose(WebSocketConnection* conn) = 0;
};


class RWebApplication : public WebApplication {
private:
  Rcpp::Function _onHeaders;
  Rcpp::Function _onBodyData;
  Rcpp::Function _onRequest;
  Rcpp::Function _onWSOpen;
  Rcpp::Function _onWSMessage;
  Rcpp::Function _onWSClose;

public:
  RWebApplication(Rcpp::Function onHeaders,
                  Rcpp::Function onBodyData,
                  Rcpp::Function onRequest,
                  Rcpp::Function onWSOpen,
                  Rcpp::Function onWSMessage,
                  Rcpp::Function onWSClose) :
    _onHeaders(onHeaders), _onBodyData(onBodyData), _onRequest(onRequest),
    _onWSOpen(onWSOpen), _onWSMessage(onWSMessage), _onWSClose(onWSClose) {
    ASSERT_MAIN_THREAD()
  }

  virtual ~RWebApplication() {
    // The Functions need to be deleted on the main thread because their
    // destructors call R's memory management functions.
    ASSERT_MAIN_THREAD()
  }

  virtual void onHeaders(HttpRequest* pRequest, boost::function<void(HttpResponse*)> callback);
  virtual void onBodyData(HttpRequest* pRequest,
                          const char* data, size_t len,
                          boost::function<void(HttpResponse*)> errorCallback);
  virtual void getResponse(HttpRequest* request, boost::function<void(HttpResponse*)> callback);
  virtual void onWSOpen(HttpRequest* pRequest, boost::function<void(void)> error_callback);
  virtual void onWSMessage(WebSocketConnection* conn,
                           bool binary, const char* data, size_t len,
                           boost::function<void(void)> error_callback);
  virtual void onWSClose(WebSocketConnection* conn);
};


#endif // WEBAPPLICATION_HPP
