#ifndef HTTP_HPP
#define HTTP_HPP

#include <uv.h>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include "webapplication.h"
#include "websockets.h"
#include "callbackqueue.h"
#include "utils.h"

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
  uv_stream_t** pServer, uv_mutex_t* mutex, uv_cond_t* cond);

void createTcpServerSync(uv_loop_t* loop, const std::string& host, int port,
  boost::shared_ptr<WebApplication> pWebApplication, CallbackQueue* background_queue,
  uv_stream_t** pServer, uv_mutex_t* mutex, uv_cond_t* cond);

void freeServer(uv_stream_t* pServer);
bool runNonBlocking(uv_loop_t* loop);


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
template <typename T>
Rcpp::XPtr<boost::shared_ptr<T>,
          Rcpp::PreserveStorage,
          Rcpp::standard_delete_finalizer<boost::shared_ptr<T>>,
          true> externalize_shared_ptr(boost::shared_ptr<T> obj)
{
  boost::shared_ptr<T>* obj_copy = new boost::shared_ptr<T>(obj);

  Rcpp::XPtr<boost::shared_ptr<T>,
             Rcpp::PreserveStorage,
             Rcpp::standard_delete_finalizer<boost::shared_ptr<T>>,
             true>
            obj_xptr(obj_copy, true);

  return obj_xptr;
}

// Given an XPtr to a shared_ptr, return the shared_ptr. This does not affect
// the shared_ptr's ref count.
template <typename T>
boost::shared_ptr<T> internalize_shared_ptr(
  Rcpp::XPtr<boost::shared_ptr<T>,
             Rcpp::PreserveStorage,
             Rcpp::standard_delete_finalizer<boost::shared_ptr<T>>,
             true> obj_xptr)
{
  boost::shared_ptr<T>* obj_copy = obj_xptr.get();
  // Return the shared pointer's value
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
