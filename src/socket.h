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
  std::vector<HttpRequest*> connections;

  Socket() {
  }

  void addConnection(HttpRequest* request);
  void removeConnection(HttpRequest* request);

  virtual ~Socket();
  virtual void destroy();
};

#endif // SOCKET_HPP
