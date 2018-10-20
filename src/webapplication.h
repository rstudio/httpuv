#ifndef WEBAPPLICATION_HPP
#define WEBAPPLICATION_HPP

#include <boost/function.hpp>
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
  virtual void onHeaders(boost::shared_ptr<HttpRequest> pRequest,
                         boost::function<void(boost::shared_ptr<HttpResponse>)> callback) = 0;
  virtual void onBodyData(boost::shared_ptr<HttpRequest> pRequest,
                          boost::shared_ptr<std::vector<char> > data,
                          boost::function<void(boost::shared_ptr<HttpResponse>)> errorCallback) = 0;
  virtual void getResponse(boost::shared_ptr<HttpRequest> request,
                           boost::function<void(boost::shared_ptr<HttpResponse>)> callback) = 0;
  virtual void onWSOpen(boost::shared_ptr<HttpRequest> pRequest,
                        boost::function<void(void)> error_callback) = 0;
  virtual void onWSMessage(boost::shared_ptr<WebSocketConnection>,
                           bool binary,
                           boost::shared_ptr<std::vector<char> > data,
                           boost::function<void(void)> error_callback) = 0;
  virtual void onWSClose(boost::shared_ptr<WebSocketConnection>) = 0;

  virtual boost::shared_ptr<HttpResponse> staticFileResponse(
    boost::shared_ptr<HttpRequest> pRequest) = 0;
  virtual StaticPathList& getStaticPathList() = 0;
};


class RWebApplication : public WebApplication {
private:
  Rcpp::Function _onHeaders;
  Rcpp::Function _onBodyData;
  Rcpp::Function _onRequest;
  Rcpp::Function _onWSOpen;
  Rcpp::Function _onWSMessage;
  Rcpp::Function _onWSClose;
  // Note this differs from the public getStaticPathList function - this is
  // the R function passed in that is used for initialization only.
  Rcpp::Function _getStaticPaths;

  StaticPathList _staticPathList;

public:
  RWebApplication(Rcpp::Function onHeaders,
                  Rcpp::Function onBodyData,
                  Rcpp::Function onRequest,
                  Rcpp::Function onWSOpen,
                  Rcpp::Function onWSMessage,
                  Rcpp::Function onWSClose,
                  Rcpp::Function getStaticPaths);

  virtual ~RWebApplication() {
    ASSERT_MAIN_THREAD()
  }

  virtual void onHeaders(boost::shared_ptr<HttpRequest> pRequest,
                         boost::function<void(boost::shared_ptr<HttpResponse>)> callback);
  virtual void onBodyData(boost::shared_ptr<HttpRequest> pRequest,
                          boost::shared_ptr<std::vector<char> > data,
                          boost::function<void(boost::shared_ptr<HttpResponse>)> errorCallback);
  virtual void getResponse(boost::shared_ptr<HttpRequest> request,
                           boost::function<void(boost::shared_ptr<HttpResponse>)> callback);
  virtual void onWSOpen(boost::shared_ptr<HttpRequest> pRequest,
                        boost::function<void(void)> error_callback);
  virtual void onWSMessage(boost::shared_ptr<WebSocketConnection> conn,
                           bool binary,
                           boost::shared_ptr<std::vector<char> > data,
                           boost::function<void(void)> error_callback);
  virtual void onWSClose(boost::shared_ptr<WebSocketConnection> conn);

  virtual boost::shared_ptr<HttpResponse> staticFileResponse(
    boost::shared_ptr<HttpRequest> pRequest);
  virtual StaticPathList& getStaticPathList();
};


#endif // WEBAPPLICATION_HPP
