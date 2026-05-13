#ifndef WEBAPPLICATION_HPP
#define WEBAPPLICATION_HPP

#include "staticpath.h"
#include "thread.h"
#include "websockets.h"
#include <functional>
#include <uv.h>

#include "cpp11.hpp"

using namespace cpp11;

class HttpRequest;
class HttpResponse;

class WebApplication {
public:
  virtual ~WebApplication() {}
  virtual void
  onHeaders(std::shared_ptr<HttpRequest> pRequest,
            std::function<void(std::shared_ptr<HttpResponse>)> callback) = 0;
  virtual void onBodyData(
      std::shared_ptr<HttpRequest> pRequest,
      std::shared_ptr<std::vector<char>> data,
      std::function<void(std::shared_ptr<HttpResponse>)> errorCallback) = 0;
  virtual void
  getResponse(std::shared_ptr<HttpRequest> request,
              std::function<void(std::shared_ptr<HttpResponse>)> callback) = 0;
  virtual void onWSOpen(std::shared_ptr<HttpRequest> pRequest,
                        std::function<void(void)> error_callback) = 0;
  virtual void onWSMessage(std::shared_ptr<WebSocketConnection>, bool binary,
                           std::shared_ptr<std::vector<char>> data,
                           std::function<void(void)> error_callback) = 0;
  virtual void onWSClose(std::shared_ptr<WebSocketConnection>) = 0;

  virtual std::shared_ptr<HttpResponse>
  staticFileResponse(std::shared_ptr<HttpRequest> pRequest) = 0;
  virtual StaticPathManager &getStaticPathManager() = 0;
};

class RWebApplication : public WebApplication {
private:
  sexp _onHeaders;
  function _onBodyData;
  function _onRequest;
  function _onWSOpen;
  function _onWSMessage;
  function _onWSClose;

  StaticPathManager _staticPathManager;

public:
  RWebApplication(sexp onHeaders, function onBodyData, function onRequest,
                  function onWSOpen, function onWSMessage, function onWSClose,
                  list staticPaths, list staticPathOptions);

  virtual ~RWebApplication() { ASSERT_MAIN_THREAD() }

  virtual void
  onHeaders(std::shared_ptr<HttpRequest> pRequest,
            std::function<void(std::shared_ptr<HttpResponse>)> callback);
  virtual void
  onBodyData(std::shared_ptr<HttpRequest> pRequest,
             std::shared_ptr<std::vector<char>> data,
             std::function<void(std::shared_ptr<HttpResponse>)> errorCallback);
  virtual void
  getResponse(std::shared_ptr<HttpRequest> request,
              std::function<void(std::shared_ptr<HttpResponse>)> callback);
  virtual void onWSOpen(std::shared_ptr<HttpRequest> pRequest,
                        std::function<void(void)> error_callback);
  virtual void onWSMessage(std::shared_ptr<WebSocketConnection> conn,
                           bool binary, std::shared_ptr<std::vector<char>> data,
                           std::function<void(void)> error_callback);
  virtual void onWSClose(std::shared_ptr<WebSocketConnection> conn);

  virtual std::shared_ptr<HttpResponse>
  staticFileResponse(std::shared_ptr<HttpRequest> pRequest);
  virtual StaticPathManager &getStaticPathManager();
};

#endif // WEBAPPLICATION_HPP
