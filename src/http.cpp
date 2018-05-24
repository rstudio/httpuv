#include "http.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "callbackqueue.h"
#include "socket.h"
#include "utils.h"
#include "thread.h"
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>


// TODO: Streaming response body (with chunked transfer encoding)
// TODO: Fast/easy use of files as response body

void on_request(uv_stream_t* handle, int status) {
  ASSERT_BACKGROUND_THREAD()
  if (status) {
    err_printf("connection error: %s\n", uv_strerror(status));
    return;
  }

  // Copy the shared_ptr
  boost::shared_ptr<Socket> pSocket(*(boost::shared_ptr<Socket>*)handle->data);
  CallbackQueue* bg_queue = pSocket->background_queue;

  // Freed by HttpRequest itself when close() is called, which
  // can occur on EOF, error, or when the Socket is destroyed
  boost::shared_ptr<HttpRequest> req = createHttpRequest(
    handle->loop, pSocket->pWebApplication, pSocket, bg_queue
  );

  int r = uv_accept(handle, req->handle());
  if (r) {
    err_printf("accept: %s\n", uv_strerror(r));
    return;
  }

  req->handleRequest();

}

uv_stream_t* createPipeServer(uv_loop_t* pLoop, const std::string& name,
  int mask, boost::shared_ptr<WebApplication> pWebApplication,
  CallbackQueue* background_queue)
{
  ASSERT_BACKGROUND_THREAD()

  // We own pWebApplication. It will be destroyed by the socket but if in
  // the future we have failure cases that stop execution before we get
  // that far, we MUST delete pWebApplication ourselves.

  boost::shared_ptr<Socket> pSocket = boost::make_shared<Socket>(
    pWebApplication, background_queue
  );

  // TODO: Handle error
  uv_pipe_init(pLoop, &pSocket->handle.pipe, true);
  pSocket->handle.isTcp = false;
  // data is a pointer to the shared_ptr. This is necessary because the
  // uv_stream_t.data field is a void*.
  pSocket->handle.stream.data = new boost::shared_ptr<Socket>(pSocket);

  mode_t oldMask = 0;
  if (mask >= 0)
    oldMask = umask(mask);
  int r = uv_pipe_bind(&pSocket->handle.pipe, name.c_str());
  if (mask >= 0)
    umask(oldMask);

  if (r) {
    delete (boost::shared_ptr<Socket>*)pSocket->handle.stream.data;
    return NULL;
  }
  r = uv_listen((uv_stream_t*)&pSocket->handle.stream, 128, &on_request);
  if (r) {
    delete (boost::shared_ptr<Socket>*)pSocket->handle.stream.data;
    return NULL;
  }

  return &pSocket->handle.stream;
}

// A wrapper for createPipeServer. The main thread schedules this to run on
// the background thread, then waits for this to finish, using a barrier.
void createPipeServerSync(uv_loop_t* loop, const std::string& name,
  int mask, boost::shared_ptr<WebApplication> pWebApplication,
  CallbackQueue* background_queue,
  uv_stream_t** pServer, Barrier* blocker)
{
  ASSERT_BACKGROUND_THREAD()

  *pServer = createPipeServer(loop, name, mask, pWebApplication, background_queue);

  // Tell the main thread that the server is ready
  blocker->wait();
}

uv_stream_t* createTcpServer(uv_loop_t* pLoop, const std::string& host,
  int port, boost::shared_ptr<WebApplication> pWebApplication,
  CallbackQueue* background_queue)
{
  ASSERT_BACKGROUND_THREAD()

  // We own pWebApplication. It will be destroyed by the socket but if in
  // the future we have failure cases that stop execution before we get
  // that far, we MUST delete pWebApplication ourselves.

  boost::shared_ptr<Socket> pSocket = boost::make_shared<Socket>(
    pWebApplication, background_queue
  );

  // TODO: Handle error
  uv_tcp_init(pLoop, &pSocket->handle.tcp);
  pSocket->handle.isTcp = true;
  // data is a pointer to the shared_ptr. This is necessary because the
  // uv_stream_t.data field is a void*.
  pSocket->handle.stream.data = new boost::shared_ptr<Socket>(pSocket);

  int r;
  // Lifetime of these needs to encompass use of pAddress in uv_tcp_bind()
  struct sockaddr_in6 addr6;
  struct sockaddr_in  addr4;
  sockaddr* pAddress;
  int family = ip_family(host);
  if (family == AF_INET6) {
    r = uv_ip6_addr(host.c_str(), port, &addr6);
    pAddress = reinterpret_cast<sockaddr*>(&addr6);
  } else if (family == AF_INET){
    r = uv_ip4_addr(host.c_str(), port, &addr4);
    pAddress = reinterpret_cast<sockaddr*>(&addr4);
  } else {
    r = 1;
    err_printf("%s is not a valid IPv4 or IPv6 address.\n", host.c_str());
  }

  if (r) {
    // It's important that close() is explicitly called, so that the uv_tcp_t is cleaned up
    pSocket->close();
    return NULL;
  }

  r = uv_tcp_bind(&pSocket->handle.tcp, pAddress, 0);

  if (r) {
    // It's important that close() is explicitly called, so that the uv_tcp_t is cleaned up
    pSocket->close();
    return NULL;
  }
  r = uv_listen((uv_stream_t*)&pSocket->handle.stream, 128, &on_request);
  if (r) {
    // It's important that close() is explicitly called, so that the uv_tcp_t is cleaned up
    pSocket->close();
    return NULL;
  }

  return &pSocket->handle.stream;
}

// A wrapper for createTcpServer. The main thread schedules this to run on the
// background thread, then waits for this to finish, using a barrier.
void createTcpServerSync(uv_loop_t* pLoop, const std::string& host,
  int port, boost::shared_ptr<WebApplication> pWebApplication,
  CallbackQueue* background_queue,
  uv_stream_t** pServer, Barrier* blocker)
{
  ASSERT_BACKGROUND_THREAD()

  *pServer = createTcpServer(pLoop, host, port, pWebApplication, background_queue);

  // Tell the main thread that the server is ready
  blocker->wait();
}


void freeServer(uv_stream_t* pHandle) {
  ASSERT_BACKGROUND_THREAD()
  // TODO: Check if server is still running?
  boost::shared_ptr<Socket>* ppSocket = (boost::shared_ptr<Socket>*)pHandle->data;
  (*ppSocket)->close();
  // ppSocket gets deleted in a callback in close()
}
