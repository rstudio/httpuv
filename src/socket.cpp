#include "socket.h"
#include "httprequest.h"
#include <later_api.h>
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

void delete_webApplication(void* pWebApplication) {
  try {
    delete reinterpret_cast<WebApplication*>(pWebApplication);
  } catch(...) {}
}

Socket::~Socket() {
  // Need to delete pWebApplication on the main thread because it contains
  // Rcpp::Function objects. We use our own callback instead of
  // delete_cb_main() because it needs to be wrapped in try-catch.
  later::later(delete_webApplication, pWebApplication, 0);
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
