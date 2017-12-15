#ifndef HTTP_HPP
#define HTTP_HPP

#include <uv.h>
#include "webapplication.h"
#include "websockets.h"
#include "callbackqueue.h"

typedef struct {
  union {
    uv_stream_t stream;
    uv_tcp_t tcp;
    uv_pipe_t pipe;
  };
  bool isTcp;
} VariantHandle;

struct Address {
  std::string host;
  unsigned short port;

  Address() : port(0) {
  }
};

class Socket;

uv_stream_t* createPipeServer(uv_loop_t* loop, const std::string& name,
  int mask, WebApplication* pWebApplication);
uv_stream_t* createTcpServer(uv_loop_t* loop, const std::string& host, int port,
  WebApplication* pWebApplication);
void createPipeServerSync(uv_loop_t* loop, const std::string& name,
  int mask, WebApplication* pWebApplication, CallbackQueue* background_queue,
  uv_stream_t** pServer, uv_barrier_t* blocker);
void createTcpServerSync(uv_loop_t* loop, const std::string& host, int port,
  WebApplication* pWebApplication, CallbackQueue* background_queue,
  uv_stream_t** pServer, uv_barrier_t* blocker);
void freeServer(uv_stream_t* pServer);
bool runNonBlocking(uv_loop_t* loop);

template <typename T>
std::string externalize(T* pServer) {
  std::ostringstream os;
  os << reinterpret_cast<uintptr_t>(pServer);
  return os.str();
}
template <typename T>
T* internalize(std::string serverHandle) {
  std::istringstream is(serverHandle);
  uintptr_t result;
  is >> result;
  return reinterpret_cast<T*>(result);
}

#endif // HTTP_HPP
