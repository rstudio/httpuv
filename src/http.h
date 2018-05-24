#ifndef HTTP_HPP
#define HTTP_HPP

#include "libuv/include/uv.h"
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
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
  boost::shared_ptr<WebApplication> pWebApplication);

uv_stream_t* createTcpServer(uv_loop_t* loop, const std::string& host, int port,
  boost::shared_ptr<WebApplication> pWebApplication);

void createPipeServerSync(uv_loop_t* loop, const std::string& name, int mask,
  boost::shared_ptr<WebApplication> pWebApplication, CallbackQueue* background_queue,
  uv_stream_t** pServer, Barrier* blocker);

void createTcpServerSync(uv_loop_t* loop, const std::string& host, int port,
  boost::shared_ptr<WebApplication> pWebApplication, CallbackQueue* background_queue,
  uv_stream_t** pServer, Barrier* blocker);

void freeServer(uv_stream_t* pServer);
bool runNonBlocking(uv_loop_t* loop);


// NOTE: externalize/internalize_shared_ptr were originally template functions
// but were made into non-template functions because gcc 4.4.7 (used on RHEL
// 6) gives the following error with the templated versions:
//   sorry, unimplemented: mangling template_id_expr
// This was due to a bug in gcc which was fixed in later versions.
//   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=38600

// externalize_shared_ptr is used to pass a shared_ptr to R, and have its
// lifetime be tied to the R external pointer object. This function creates a
// copy of the shared_ptr (incrementing the shared_ptr's target's refcount)
// using `new`, and puts it inside of the XPtr. When the XPtr is garbage
// collected, the shared_ptr is deleted, which decrements the refcount.
//
// As long as R has the XPtr object, the shared_ptr's target won't be deleted.
// Also, when the XPtr gets GC'd, the shared_ptr will get deleted, and if the
// refcount goes to 0, then the target object will be deleted (or, if it has a
// deleter, that will be called). This means that the target object could be
// deleted from the main thread due to a GC event in R.
//
// The reason we need the explicit Xptr type is because we want to set the last
// argument (finalizeOnExit) to true.
inline Rcpp::XPtr<boost::shared_ptr<WebSocketConnection>,
                  Rcpp::PreserveStorage,
                  auto_deleter_background<boost::shared_ptr<WebSocketConnection> >,
                  true> externalize_shared_ptr(boost::shared_ptr<WebSocketConnection> obj)
{
  ASSERT_MAIN_THREAD()
  boost::shared_ptr<WebSocketConnection>* obj_copy = new boost::shared_ptr<WebSocketConnection>(obj);

  Rcpp::XPtr<boost::shared_ptr<WebSocketConnection>,
             Rcpp::PreserveStorage,
             auto_deleter_background<boost::shared_ptr<WebSocketConnection> >,
             true> obj_xptr(obj_copy, true);

  return obj_xptr;
}

// Given an XPtr to a shared_ptr, return a copy of the shared_ptr. This
// increases the shared_ptr's ref count by one.
inline boost::shared_ptr<WebSocketConnection> internalize_shared_ptr(
  Rcpp::XPtr<boost::shared_ptr<WebSocketConnection>,
             Rcpp::PreserveStorage,
             auto_deleter_background<boost::shared_ptr<WebSocketConnection> >,
             true> obj_xptr)
{
  ASSERT_MAIN_THREAD()
  boost::shared_ptr<WebSocketConnection>* obj_copy = obj_xptr.get();
  // Return a copy of the shared pointer.
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
