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


// TODO: Streaming response body (with chunked transfer encoding)
// TODO: Fast/easy use of files as response body

void on_request(uv_stream_t* handle, int status) {
  ASSERT_BACKGROUND_THREAD()
  if (status) {
    err_printf("connection error: %s\n", uv_strerror(status));
    return;
  }

  Socket* pSocket = (Socket*)handle->data;
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
  int mask, WebApplication* pWebApplication, CallbackQueue* background_queue) {

  // We own pWebApplication. It will be destroyed by the socket but if in
  // the future we have failure cases that stop execution before we get
  // that far, we MUST delete pWebApplication ourselves.

  // Deletes itself when destroy() is called, which occurs in freeServer()
  Socket* pSocket = new Socket(pWebApplication, background_queue);
  // TODO: Handle error
  uv_pipe_init(pLoop, &pSocket->handle.pipe, true);
  pSocket->handle.isTcp = false;
  pSocket->handle.stream.data = pSocket;

  mode_t oldMask = 0;
  if (mask >= 0)
    oldMask = umask(mask);
  int r = uv_pipe_bind(&pSocket->handle.pipe, name.c_str());
  if (mask >= 0)
    umask(oldMask);

  if (r) {
    pSocket->destroy();
    return NULL;
  }
  r = uv_listen((uv_stream_t*)&pSocket->handle.stream, 128, &on_request);
  if (r) {
    pSocket->destroy();
    return NULL;
  }

  return &pSocket->handle.stream;
}

// A wrapper for createPipeServer. The main thread schedules this to run on
// the background thread, then waits for this to finish, using a barrier.
void createPipeServerSync(uv_loop_t* loop, const std::string& name,
  int mask, WebApplication* pWebApplication, CallbackQueue* background_queue,
  uv_stream_t** pServer, uv_mutex_t* mutex, uv_cond_t* cond)
{
  ASSERT_BACKGROUND_THREAD()
  *pServer = createPipeServer(loop, name, mask, pWebApplication, background_queue);

  // Tell the main thread that the server is ready
  uv_mutex_lock(mutex);
  uv_cond_signal(cond);
  uv_mutex_unlock(mutex);
}


uv_stream_t* createTcpServer(uv_loop_t* pLoop, const std::string& host,
  int port, WebApplication* pWebApplication, CallbackQueue* background_queue) {

  // We own pWebApplication. It will be destroyed by the socket but if in
  // the future we have failure cases that stop execution before we get
  // that far, we MUST delete pWebApplication ourselves.

  // Deletes itself when destroy() is called, in io_thread()
  Socket* pSocket = new Socket(pWebApplication, background_queue);
  // TODO: Handle error
  uv_tcp_init(pLoop, &pSocket->handle.tcp);
  pSocket->handle.isTcp = true;
  pSocket->handle.stream.data = pSocket;

  struct sockaddr_in address = {0};
  int r = uv_ip4_addr(host.c_str(), port, &address);
  if (r) {
    pSocket->destroy();
    return NULL;
  }
  r = uv_tcp_bind(&pSocket->handle.tcp, (sockaddr*)&address, 0);
  if (r) {
    pSocket->destroy();
    return NULL;
  }
  r = uv_listen((uv_stream_t*)&pSocket->handle.stream, 128, &on_request);
  if (r) {
    pSocket->destroy();
    return NULL;
  }

  return &pSocket->handle.stream;
}

// A wrapper for createTcpServer. The main thread schedules this to run on the
// background thread, then waits for this to finish, using a barrier.
void createTcpServerSync(uv_loop_t* pLoop, const std::string& host,
  int port, WebApplication* pWebApplication, CallbackQueue* background_queue,
  uv_stream_t** pServer, uv_mutex_t* mutex, uv_cond_t* cond)
{
  ASSERT_BACKGROUND_THREAD()
  *pServer = createTcpServer(pLoop, host, port, pWebApplication, background_queue);

  // Tell the main thread that the server is ready
  uv_mutex_lock(mutex);
  uv_cond_signal(cond);
  uv_mutex_unlock(mutex);
}


void freeServer(uv_stream_t* pHandle) {
  ASSERT_BACKGROUND_THREAD()
  // TODO: Check if server is still running?
  Socket* pSocket = (Socket*)pHandle->data;
  pSocket->destroy();
}
