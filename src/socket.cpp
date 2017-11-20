#include "socket.h"
#include "httprequest.h"
#include <uv.h>

void on_Socket_close(uv_handle_t* pHandle);

void Socket::addConnection(HttpRequest* request) {
  connections.push_back(request);
}

void Socket::removeConnection(HttpRequest* request) {
  connections.erase(
    std::remove(connections.begin(), connections.end(), request),
    connections.end());
}

Socket::~Socket() {
  try {
    delete pWebApplication;
  } catch(...) {}
}

void Socket::destroy() {
  for (std::vector<HttpRequest*>::reverse_iterator it = connections.rbegin();
    it != connections.rend();
    it++) {

    // std::cerr << "Request close on " << *it << std::endl;
    (*it)->close();
  }
  uv_close(toHandle(&handle.stream), on_Socket_close);
}

void on_Socket_close(uv_handle_t* pHandle) {
  StreamHandleData* handle_data = (StreamHandleData*)pHandle->data;
  delete handle_data->socket;
}
