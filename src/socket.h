#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "http.h"
#include <uv.h>

class HttpRequest;
class WebApplication;

class Socket {
public:
  VariantHandle handle;
  WebApplication* pWebApplication;
  CallbackQueue* background_queue;
  std::vector<HttpRequest*> connections;

  Socket(WebApplication* pWebApplication, CallbackQueue* background_queue)
    : pWebApplication(pWebApplication), background_queue(background_queue)
  {
  }

  void addConnection(HttpRequest* request);
  void removeConnection(HttpRequest* request);

  virtual ~Socket();
  virtual void destroy();
};

#endif // SOCKET_HPP
