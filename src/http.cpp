#include "http.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "socket.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <iostream>
#include <sstream>


// TODO: Streaming response body (with chunked transfer encoding)
// TODO: Fast/easy use of files as response body

void on_request(uv_stream_t* handle, int status) {
  ASSERT_BACKGROUND_THREAD()
  if (status) {
    REprintf("connection error: %s\n", uv_strerror(status));
    return;
  }

  Socket* pSocket = (Socket*)handle->data;

  // Freed by HttpRequest itself when close() is called, which
  // can occur on EOF, error, or when the Socket is destroyed
  HttpRequest* req = new HttpRequest(
    handle->loop, pSocket->pWebApplication, pSocket);

  int r = uv_accept(handle, req->handle());
  if (r) {
    REprintf("accept: %s\n", uv_strerror(r));
    delete req;
    return;
  }

  req->handleRequest();

}

uv_stream_t* createPipeServer(uv_loop_t* pLoop, const std::string& name,
  int mask, WebApplication* pWebApplication) {

  // We own pWebApplication. It will be destroyed by the socket but if in
  // the future we have failure cases that stop execution before we get
  // that far, we MUST delete pWebApplication ourselves.

  // Deletes itself when destroy() is called, which occurs in freeServer()
  Socket* pSocket = new Socket();
  // TODO: Handle error
  uv_pipe_init(pLoop, &pSocket->handle.pipe, true);
  pSocket->handle.isTcp = false;
  pSocket->handle.stream.data = pSocket;
  pSocket->pWebApplication = pWebApplication;

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

uv_stream_t* createTcpServer(uv_loop_t* pLoop, const std::string& host,
  int port, WebApplication* pWebApplication) {

  // We own pWebApplication. It will be destroyed by the socket but if in
  // the future we have failure cases that stop execution before we get
  // that far, we MUST delete pWebApplication ourselves.

  // Deletes itself when destroy() is called, which occurs in freeServer()
  Socket* pSocket = new Socket();
  // TODO: Handle error
  uv_tcp_init(pLoop, &pSocket->handle.tcp);
  pSocket->handle.isTcp = true;
  pSocket->handle.stream.data = pSocket;
  pSocket->pWebApplication = pWebApplication;

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
void freeServer(uv_stream_t* pHandle) {
  uv_loop_t* loop = pHandle->loop;
  Socket* pSocket = (Socket*)pHandle->data;
  pSocket->destroy();

  runNonBlocking(loop);
}
bool runNonBlocking(uv_loop_t* loop) {
  return uv_run(loop, UV_RUN_NOWAIT);
}
