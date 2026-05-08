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


// NOTE: externalize/internalize_shared_ptr were originally template functions
// but were made into non-template functions because gcc 4.4.7 (used on RHEL
// 6) gives the following error with the templated versions:
//   sorry, unimplemented: mangling template_id_expr
// This was due to a bug in gcc which was fixed in later versions.
//   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=38600

// Finalizer for WebSocketConnection external pointers.  Schedules deletion
// on the background thread so that R's GC (which runs on the main thread)
// does not directly delete an object that must be destroyed on the bg thread.
inline void ws_conn_xptr_finalizer(SEXP xptr) {
  std::shared_ptr<WebSocketConnection>* obj =
    (std::shared_ptr<WebSocketConnection>*)R_ExternalPtrAddr(xptr);
  if (obj) {
    auto_deleter_background(obj);
    R_ClearExternalPtr(xptr);
  }
}

// externalize_shared_ptr is used to pass a shared_ptr to R, and have its
// lifetime be tied to the R external pointer object.  This function allocates
// a copy of the shared_ptr on the heap and wraps it in an R external pointer
// with a finalizer that schedules deletion on the background thread.
inline SEXP externalize_shared_ptr(std::shared_ptr<WebSocketConnection> obj)
{
  ASSERT_MAIN_THREAD()
  std::shared_ptr<WebSocketConnection>* obj_copy = new std::shared_ptr<WebSocketConnection>(obj);
  SEXP xptr = R_MakeExternalPtr(obj_copy, R_NilValue, R_NilValue);
  R_RegisterCFinalizer(xptr, ws_conn_xptr_finalizer);
  return xptr;
}

// Given an R external pointer wrapping a shared_ptr, return a copy of the
// shared_ptr (incrementing the ref count by one).
inline std::shared_ptr<WebSocketConnection> internalize_shared_ptr(SEXP xptr)
{
  ASSERT_MAIN_THREAD()
  std::shared_ptr<WebSocketConnection>* obj_copy =
    (std::shared_ptr<WebSocketConnection>*)R_ExternalPtrAddr(xptr);
  return *obj_copy;
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
