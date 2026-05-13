#ifndef HTTP_HPP
#define HTTP_HPP

#include <uv.h>
#include <memory>
#include <functional>
#include "webapplication.h"
#include "websockets.h"
#include "callbackqueue.h"
#include "utils.h"
#include "auto_deleter.h"

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

uv_stream_t* createPipeServer(uv_loop_t* loop, const std::string& name, int mask,
  std::shared_ptr<WebApplication> pWebApplication);

uv_stream_t* createTcpServer(uv_loop_t* loop, const std::string& host, int port,
  std::shared_ptr<WebApplication> pWebApplication);

void createPipeServerSync(uv_loop_t* loop, const std::string& name, int mask,
  std::shared_ptr<WebApplication> pWebApplication, bool quiet,
  CallbackQueue* background_queue,
  uv_stream_t** pServer, std::shared_ptr<Barrier> blocker);

void createTcpServerSync(uv_loop_t* loop, const std::string& host, int port,
  std::shared_ptr<WebApplication> pWebApplication, bool quiet,
  CallbackQueue* background_queue,
  uv_stream_t** pServer, std::shared_ptr<Barrier> blocker);

void freeServer(uv_stream_t* pServer);
bool runNonBlocking(uv_loop_t* loop);


// externalize_shared_ptr is used to pass a shared_ptr to R, and have its
// lifetime be tied to the R external pointer object. This function creates a
// copy of the shared_ptr (incrementing the shared_ptr's target's refcount)
// using `new`, and puts it inside of the external_pointer. When the external_pointer is
// garbage collected, the shared_ptr is deleted, which decrements the refcount.
//
// As long as R has the external_pointer object, the shared_ptr's target won't be deleted.
// Also, when the external_pointer gets GC'd, the shared_ptr will get deleted, and if the
// refcount goes to 0, then the target object will be deleted (or, if it has a
// deleter, that will be called). This means that the target object could be
// deleted from the main thread due to a GC event in R.
inline external_pointer<std::shared_ptr<WebSocketConnection>,
                        auto_deleter_background<std::shared_ptr<WebSocketConnection>>>
externalize_shared_ptr(std::shared_ptr<WebSocketConnection> obj)
{
  ASSERT_MAIN_THREAD()
  std::shared_ptr<WebSocketConnection>* obj_copy = new std::shared_ptr<WebSocketConnection>(obj);
  return external_pointer<std::shared_ptr<WebSocketConnection>,
                          auto_deleter_background<std::shared_ptr<WebSocketConnection>>>(obj_copy, true, true);
}


template <typename T>
std::string externalize_str(T* pServer) {
  std::ostringstream os;
  os << reinterpret_cast<uintptr_t>(pServer);
  return os.str();
}

template <typename T>
T* internalize_str(std::string serverHandle) {
  std::istringstream is(serverHandle);
  uintptr_t result;
  is >> result;
  return reinterpret_cast<T*>(result);
}

#endif // HTTP_HPP
