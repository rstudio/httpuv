#ifndef WEBAPPLICATION_HPP
#define WEBAPPLICATION_HPP

#include <functional>
#include <uv.h>
#include <Rinternals.h>
#ifdef length
# undef length
#endif
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
  SEXP _onHeaders;
  SEXP _onBodyData;
  SEXP _onRequest;
  SEXP _onWSOpen;
  SEXP _onWSMessage;
  SEXP _onWSClose;

  StaticPathManager _staticPathManager;

public:
  RWebApplication(SEXP onHeaders,
                  SEXP onBodyData,
                  SEXP onRequest,
                  SEXP onWSOpen,
                  SEXP onWSMessage,
                  SEXP onWSClose,
                  SEXP staticPaths,
                  SEXP staticPathOptions);

  virtual ~RWebApplication() {
    ASSERT_MAIN_THREAD()
    R_ReleaseObject(_onHeaders);
    R_ReleaseObject(_onBodyData);
    R_ReleaseObject(_onRequest);
    R_ReleaseObject(_onWSOpen);
    R_ReleaseObject(_onWSMessage);
    R_ReleaseObject(_onWSClose);
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
