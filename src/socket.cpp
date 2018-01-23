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

void Socket_deleter(Socket* pSocket) {
  ASSERT_BACKGROUND_THREAD()
  trace("Socket_deleter");
  for (std::vector<boost::shared_ptr<HttpRequest>>::reverse_iterator it = pSocket->connections.rbegin();
    it != pSocket->connections.rend();
    it++) {

    // std::cerr << "Request close on " << *it << std::endl;
    (*it)->close();
  }

  uv_handle_t* pHandle = toHandle(&pSocket->handle.stream);
  // pHandle->data is currently the pointer to the shared_ptr<Socket> to this
  // Socket, but when this deleter has been called, the shared_ptr's refcount
  // has dropped to zero so it's no longer useful. We need to pass the Socket*
  // to the close callback somehow, so we'll just overwrite pHandle->data.
  pHandle->data = pSocket;

  uv_close(pHandle, on_Socket_close);
}

void on_Socket_close(uv_handle_t* pHandle) {
  ASSERT_BACKGROUND_THREAD()
  trace("on_Socket_close");
  Socket* pSocket = reinterpret_cast<Socket*>(pHandle->data);
  delete pSocket;
}
