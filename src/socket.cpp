#include "socket.h"
#include "httprequest.h"
#include <later_api.h>
#include <uv.h>

void on_Socket_close(uv_handle_t* pHandle);

void Socket::addConnection(boost::shared_ptr<HttpRequest> request) {
  connections.push_back(request);
}

void Socket::removeConnection(boost::shared_ptr<HttpRequest> request) {
  connections.erase(
    std::remove(connections.begin(), connections.end(), request),
    connections.end());
}

Socket::~Socket() {
  ASSERT_BACKGROUND_THREAD()
  trace("Socket::~Socket");
}

void Socket::destroy() {
  ASSERT_BACKGROUND_THREAD()
  trace("Socket::destroy");
  for (std::vector<boost::shared_ptr<HttpRequest>>::reverse_iterator it = connections.rbegin();
    it != connections.rend();
    it++) {

    // std::cerr << "Request close on " << *it << std::endl;
    (*it)->close();
  }
  uv_close(toHandle(&handle.stream), on_Socket_close);
}

void on_Socket_close(uv_handle_t* pHandle) {
  boost::shared_ptr<Socket>* pSocket = reinterpret_cast<boost::shared_ptr<Socket>*>(pHandle->data);
  // Delete the pointer to the shared_ptr to the Socket. This may trigger
  // ~Socket if it's the last shared_ptr to the Socket.
  delete pSocket;
}
