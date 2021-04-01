#ifndef WEBAPPLICATION_HPP
#define WEBAPPLICATION_HPP

#include <functional>
#include "libuv/include/uv.h"
#include <Rcpp.h>
#include "websockets.h"
#include "thread.h"
#include "staticpath.h"

class HttpRequest;
class HttpResponse;

class WebApplication {
public:
  virtual ~WebApplication() {}
  virtual void onHeaders(std::shared_ptr<HttpRequest> pRequest,
                         std::function<void(std::shared_ptr<HttpResponse>)> callback) = 0;
  virtual void onBodyData(std::shared_ptr<HttpRequest> pRequest,
                          std::shared_ptr<std::vector<char> > data,
                          std::function<void(std::shared_ptr<HttpResponse>)> errorCallback) = 0;
  virtual void getResponse(std::shared_ptr<HttpRequest> request,
                           std::function<void(std::shared_ptr<HttpResponse>)> callback) = 0;
  virtual void onWSOpen(std::shared_ptr<HttpRequest> pRequest,
                        std::function<void(void)> error_callback) = 0;
  virtual void onWSMessage(std::shared_ptr<WebSocketConnection>,
                           bool binary,
                           std::shared_ptr<std::vector<char> > data,
                           std::function<void(void)> error_callback) = 0;
  virtual void onWSClose(std::shared_ptr<WebSocketConnection>) = 0;

  virtual std::shared_ptr<HttpResponse> staticFileResponse(
    std::shared_ptr<HttpRequest> pRequest) = 0;
  virtual StaticPathManager& getStaticPathManager() = 0;
};


class RWebApplication : public WebApplication {
private:
  Rcpp::Function _onHeaders;
  Rcpp::Function _onBodyData;
  Rcpp::Function _onRequest;
  Rcpp::Function _onWSOpen;
  Rcpp::Function _onWSMessage;
  Rcpp::Function _onWSClose;

  StaticPathManager _staticPathManager;

public:
  RWebApplication(Rcpp::Function onHeaders,
                  Rcpp::Function onBodyData,
                  Rcpp::Function onRequest,
                  Rcpp::Function onWSOpen,
                  Rcpp::Function onWSMessage,
                  Rcpp::Function onWSClose,
                  Rcpp::List     staticPaths,
                  Rcpp::List     staticPathOptions);

  virtual ~RWebApplication() {
    ASSERT_MAIN_THREAD()
  }

  virtual void onHeaders(std::shared_ptr<HttpRequest> pRequest,
                         std::function<void(std::shared_ptr<HttpResponse>)> callback);
  virtual void onBodyData(std::shared_ptr<HttpRequest> pRequest,
                          std::shared_ptr<std::vector<char> > data,
                          std::function<void(std::shared_ptr<HttpResponse>)> errorCallback);
  virtual void getResponse(std::shared_ptr<HttpRequest> request,
                           std::function<void(std::shared_ptr<HttpResponse>)> callback);
  virtual void onWSOpen(std::shared_ptr<HttpRequest> pRequest,
                        std::function<void(void)> error_callback);
  virtual void onWSMessage(std::shared_ptr<WebSocketConnection> conn,
                           bool binary,
                           std::shared_ptr<std::vector<char> > data,
                           std::function<void(void)> error_callback);
  virtual void onWSClose(std::shared_ptr<WebSocketConnection> conn);

  virtual std::shared_ptr<HttpResponse> staticFileResponse(
    std::shared_ptr<HttpRequest> pRequest);
  virtual StaticPathManager& getStaticPathManager();
};


#endif // WEBAPPLICATION_HPP
