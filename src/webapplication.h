#ifndef WEBAPPLICATION_HPP
#define WEBAPPLICATION_HPP

#include <boost/function.hpp>
#include "libuv/include/uv.h"
#include <Rcpp.h>
#include "websockets.h"
#include "thread.h"

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
  virtual bool isStaticPath(const std::string& url_path) = 0;
  virtual boost::shared_ptr<HttpResponse> staticFileResponse(
    boost::shared_ptr<HttpRequest> pRequest) = 0;
  // Get current set of static paths.
  virtual std::map<std::string, std::string> getStaticPaths() const = 0;
  virtual std::map<std::string, std::string> addStaticPaths(
    const std::map<std::string, std::string>& paths) = 0;
  virtual std::map<std::string, std::string> removeStaticPaths(
    const std::vector<std::string>& paths) = 0;
};


class RWebApplication : public WebApplication {
private:
  Rcpp::Function _onHeaders;
  Rcpp::Function _onBodyData;
  Rcpp::Function _onRequest;
  Rcpp::Function _onWSOpen;
  Rcpp::Function _onWSMessage;
  Rcpp::Function _onWSClose;
  // Note this is different from the public getStaticPaths function - this is
  // the R function passed in that is used for initialization only.
  Rcpp::Function _getStaticPaths;

  // TODO: need to be careful with this - it's created and destroyed on the
  // main thread, but when it is accessed, it is always on the background thread.
  // Probably need to lock at those times.
  std::map<std::string, std::string> _staticPaths;

  std::pair<std::string, std::string> _matchStaticPath(const std::string& url_path) const;

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

  virtual bool isStaticPath(const std::string& url_path);
  virtual boost::shared_ptr<HttpResponse> staticFileResponse(
    boost::shared_ptr<HttpRequest> pRequest);
  virtual std::map<std::string, std::string> getStaticPaths() const;
  virtual std::map<std::string, std::string> addStaticPaths(
    const std::map<std::string, std::string>& paths);
  virtual std::map<std::string, std::string> removeStaticPaths(
    const std::vector<std::string>& paths);
};


#endif // WEBAPPLICATION_HPP
