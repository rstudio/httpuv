#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <map>
#include <iomanip>
#include <signal.h>
#include <errno.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "libuv/include/uv.h"
#include "base64/base64.hpp"
#include "uvutil.h"
#include "webapplication.h"
#include "http.h"
#include "callbackqueue.h"
#include "utils.h"
#include "thread.h"
#include "httpuv.h"
#include "auto_deleter.h"
#include <Rinternals.h>


void throwError(int err,
  const std::string& prefix = std::string(),
  const std::string& suffix = std::string())
{
  ASSERT_MAIN_THREAD()
  std::string msg = prefix + uv_strerror(err) + suffix;
  throw Rcpp::exception(msg.c_str());
}

// For keeping track of all running server apps.
std::vector<uv_stream_t*> pServers;


class UVLoop {
public:
  UVLoop() : _initialized(false) {
    uv_mutex_init(&_mutex);
  };

  void ensure_initialized() {
    guard guard(_mutex);
    if (!_initialized) {
      uv_loop_init(&_loop);
      _initialized = true;
    }
  }

  uv_loop_t* get() {
    guard guard(_mutex);
    if (!_initialized) {
      throw std::runtime_error("io_loop not initialized!");
    }

    return &_loop;
  };

  void reset() {
    guard guard(_mutex);
    _initialized = false;
  }

private:
  uv_loop_t _loop;
  uv_mutex_t _mutex;
  bool _initialized;
};

// ============================================================================
// Background thread and I/O event loop
// ============================================================================

// A queue of tasks to run on the background thread. This is how the main
// thread schedules work to be done on the background thread.
CallbackQueue* background_queue;

uv_thread_t io_thread_id;
ThreadSafe<bool> io_thread_running(false);

UVLoop io_loop;
uv_async_t async_stop_io_loop;

void close_handle_cb(uv_handle_t* handle, void* arg) {
  ASSERT_BACKGROUND_THREAD()
  uv_close(handle, NULL);
}

void stop_io_loop(uv_async_t *handle) {
  ASSERT_BACKGROUND_THREAD()
  trace("stop_io_loop");
  uv_stop(io_loop.get());
}

void io_thread(void* data) {
  register_background_thread();
  Barrier* blocker = reinterpret_cast<Barrier*>(data);

  io_thread_running.set(true);

  io_loop.ensure_initialized();

  background_queue = new CallbackQueue(io_loop.get());

  // Set up async communication channels
  uv_async_init(io_loop.get(), &async_stop_io_loop, stop_io_loop);

  // Tell other thread that it can continue.
  blocker->wait();

  // Run io_loop. When it stops, this fuction continues and the thread exits.
  uv_run(io_loop.get(), UV_RUN_DEFAULT);

  trace("io_loop stopped");

  // Cleanup stuff
  uv_walk(io_loop.get(), close_handle_cb, NULL);
  uv_run(io_loop.get(), UV_RUN_ONCE);
  uv_loop_close(io_loop.get());
  io_loop.reset();
  io_thread_running.set(false);

  delete background_queue;
}

void ensure_io_thread() {
  ASSERT_MAIN_THREAD()
  if (io_thread_running.get()) {
    return;
  }

  Barrier blocker(2);
  int ret = uv_thread_create(&io_thread_id, io_thread, &blocker);
  // Wait for io_loop to be initialized before continuing
  blocker.wait();

  if (ret != 0) {
    Rcpp::stop(std::string("Error: ") + uv_strerror(ret));
  }
}


// ============================================================================
// Outgoing websocket messages
// ============================================================================

// [[Rcpp::export]]
void sendWSMessage(SEXP conn,
                   bool binary,
                   Rcpp::RObject message)
{
  ASSERT_MAIN_THREAD()
  Rcpp::XPtr<boost::shared_ptr<WebSocketConnection>,
             Rcpp::PreserveStorage,
             auto_deleter_background<boost::shared_ptr<WebSocketConnection> >,
             true> conn_xptr(conn);
  boost::shared_ptr<WebSocketConnection> wsc = internalize_shared_ptr(conn_xptr);

  Opcode mode;
  SEXP msg_sexp;
  std::vector<char>* str;

  // Efficiently copy message into a new vector<char>. There's probably a
  // cleaner way to do this.
   if (binary) {
    mode = Binary;
    msg_sexp = PROTECT(Rcpp::as<SEXP>(message));
    str = new std::vector<char>(RAW(msg_sexp), RAW(msg_sexp) + Rf_length(msg_sexp));
    UNPROTECT(1);

  } else {
    mode = Text;
    msg_sexp = PROTECT(STRING_ELT(message, 0));
    str = new std::vector<char>(CHAR(msg_sexp), CHAR(msg_sexp) + Rf_length(msg_sexp));
    UNPROTECT(1);
  }


  boost::function<void (void)> cb(
    boost::bind(&WebSocketConnection::sendWSMessage, wsc,
      mode,
      safe_vec_addr(*str),
      str->size()
    )
  );

  background_queue->push(cb);
  // Free str after data is written
  // deleter_background<std::vector<char>>(str)
  background_queue->push(boost::bind(deleter_background<std::vector<char> >, str));
}

// [[Rcpp::export]]
void closeWS(SEXP conn,
             uint16_t code,
             std::string reason)
{
  ASSERT_MAIN_THREAD()
  trace("closeWS\n");
  Rcpp::XPtr<boost::shared_ptr<WebSocketConnection>,
             Rcpp::PreserveStorage,
             auto_deleter_background<boost::shared_ptr<WebSocketConnection> >,
             true> conn_xptr(conn);
  boost::shared_ptr<WebSocketConnection> wsc = internalize_shared_ptr(conn_xptr);

  // Schedule on background thread:
  // wsc->closeWS(code, reason);
  background_queue->push(
    boost::bind(&WebSocketConnection::closeWS, wsc, code, reason)
  );
}


// [[Rcpp::export]]
Rcpp::RObject makeTcpServer(const std::string& host, int port,
                            Rcpp::Function onHeaders,
                            Rcpp::Function onBodyData,
                            Rcpp::Function onRequest,
                            Rcpp::Function onWSOpen,
                            Rcpp::Function onWSMessage,
                            Rcpp::Function onWSClose) {

  using namespace Rcpp;
  register_main_thread();

  // Deleted when owning pServer is deleted. If pServer creation fails,
  // this should be deleted when it goes out of scope.
  boost::shared_ptr<RWebApplication> pHandler(
    new RWebApplication(onHeaders, onBodyData, onRequest,
                        onWSOpen, onWSMessage, onWSClose),
    auto_deleter_main<RWebApplication>
  );

  ensure_io_thread();

  Barrier blocker(2);

  uv_stream_t* pServer;

  // Run on background thread:
  // createTcpServerSync(
  //   io_loop.get(), host.c_str(), port,
  //   boost::static_pointer_cast<WebApplication>(pHandler),
  //   background_queue, &pServer, &blocker
  // );
  background_queue->push(
    boost::bind(createTcpServerSync,
      io_loop.get(), host.c_str(), port,
      boost::static_pointer_cast<WebApplication>(pHandler),
      background_queue, &pServer, &blocker
    )
  );

  // Wait for server to be created before continuing
  blocker.wait();

  if (!pServer) {
    return R_NilValue;
  }

  pServers.push_back(pServer);

  return Rcpp::wrap(externalize_str<uv_stream_t>(pServer));
}

// [[Rcpp::export]]
Rcpp::RObject makePipeServer(const std::string& name,
                             int mask,
                             Rcpp::Function onHeaders,
                             Rcpp::Function onBodyData,
                             Rcpp::Function onRequest,
                             Rcpp::Function onWSOpen,
                             Rcpp::Function onWSMessage,
                             Rcpp::Function onWSClose) {

  using namespace Rcpp;
  register_main_thread();

  // Deleted when owning pServer is deleted. If pServer creation fails,
  // this should be deleted when it goes out of scope.
  boost::shared_ptr<RWebApplication> pHandler(
    new RWebApplication(onHeaders, onBodyData, onRequest,
                        onWSOpen, onWSMessage, onWSClose),
    auto_deleter_main<RWebApplication>
  );

  ensure_io_thread();

  Barrier blocker(2);

  uv_stream_t* pServer;

  // Run on background thread:
  // createPipeServerSync(
  //   io_loop.get(), name.c_str(), mask,
  //   boost::static_pointer_cast<WebApplication>(pHandler),
  //   background_queue, &pServer, &blocker
  // );
  background_queue->push(
    boost::bind(createPipeServerSync,
      io_loop.get(), name.c_str(), mask,
      boost::static_pointer_cast<WebApplication>(pHandler),
      background_queue, &pServer, &blocker
    )
  );

  // Wait for server to be created before continuing
  blocker.wait();

  if (!pServer) {
    return R_NilValue;
  }

  pServers.push_back(pServer);

  return Rcpp::wrap(externalize_str<uv_stream_t>(pServer));
}


void stopServer(uv_stream_t* pServer) {
  ASSERT_MAIN_THREAD()

  // Remove it from the list of running servers.
  // Note: we're removing it from the pServers list without waiting for the
  // background thread to call freeServer().
  std::vector<uv_stream_t*>::iterator pos = std::find(pServers.begin(), pServers.end(), pServer);
  if (pos != pServers.end()) {
    pServers.erase(pos);
  } else {
    throw Rcpp::exception("pServer handle not found in list of running servers.");
  }

  // Run on background thread:
  // freeServer(pServer);
  background_queue->push(
    boost::bind(freeServer, pServer)
  );
}

//' Stop a server
//' 
//' Given a handle that was returned from a previous invocation of 
//' \code{\link{startServer}} or \code{\link{startPipeServer}}, this closes all
//' open connections for that server and  unbinds the port.
//' 
//' @param handle A handle that was previously returned from
//'   \code{\link{startServer}} or \code{\link{startPipeServer}}.
//'
//' @seealso \code{\link{stopAllServers}} to stop all servers.
//'
//' @export
// [[Rcpp::export]]
void stopServer(std::string handle) {
  ASSERT_MAIN_THREAD()
  uv_stream_t* pServer = internalize_str<uv_stream_t>(handle);
  stopServer(pServer);
}

//' Stop all applications
//'
//' This will stop all applications which were created by
//' \code{\link{startServer}} or \code{\link{startPipeServer}}.
//'
//' @seealso \code{\link{stopServer}} to stop a specific server.
//'
//' @export
// [[Rcpp::export]]
void stopAllServers() {
  ASSERT_MAIN_THREAD()

  if (!io_thread_running.get())
    return;

  // Each call to stopServer also removes it from the pServers list.
  while (pServers.size() > 0) {
    stopServer(pServers[0]);
  }

  uv_async_send(&async_stop_io_loop);

  trace("io_thread stopped");
  uv_thread_join(&io_thread_id);
}

void stop_loop_timer_cb(uv_timer_t* handle) {
  uv_stop(handle->loop);
}


// ============================================================================
// Miscellaneous utility functions
// ============================================================================

// [[Rcpp::export]]
std::string base64encode(const Rcpp::RawVector& x) {
  return b64encode(x.begin(), x.end());
}

static std::string allowed = ";,/?:@&=+$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_.!~*'()";

bool isReservedUrlChar(char c) {
  switch (c) {
    case ';':
    case ',':
    case '/':
    case '?':
    case ':':
    case '@':
    case '&':
    case '=':
    case '+':
    case '$':
      return true;
    default:
      return false;
  }
}

bool needsEscape(char c, bool encodeReserved) {
  if (c >= 'a' && c <= 'z')
    return false;
  if (c >= 'A' && c <= 'Z')
    return false;
  if (c >= '0' && c <= '9')
    return false;
  if (isReservedUrlChar(c))
    return encodeReserved;
  switch (c) {
    case '-':
    case '_':
    case '.':
    case '!':
    case '~':
    case '*':
    case '\'':
    case '(':
    case ')':
      return false;
  }
  return true;
}

std::string doEncodeURI(std::string value, bool encodeReserved) {
  std::ostringstream os;
  os << std::hex << std::uppercase;
  for (std::string::const_iterator it = value.begin();
    it != value.end();
    it++) {
    
    if (!needsEscape(*it, encodeReserved)) {
      os << *it;
    } else {
      os << '%' << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(*it));
    }
  }
  return os.str();
}

//' URI encoding/decoding
//' 
//' Encodes/decodes strings using URI encoding/decoding in the same way that web
//' browsers do. The precise behaviors of these functions can be found at
//' developer.mozilla.org:
//' \href{https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURI}{encodeURI},
//' \href{https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURIComponent}{encodeURIComponent},
//' \href{https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/decodeURI}{decodeURI},
//' \href{https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/decodeURIComponent}{decodeURIComponent}
//' 
//' Intended as a faster replacement for \code{\link[utils]{URLencode}} and
//' \code{\link[utils]{URLdecode}}.
//' 
//' encodeURI differs from encodeURIComponent in that the former will not encode
//' reserved characters: \code{;,/?:@@&=+$}
//' 
//' decodeURI differs from decodeURIComponent in that it will refuse to decode
//' encoded sequences that decode to a reserved character. (If in doubt, use
//' decodeURIComponent.)
//' 
//' The only way these functions differ from web browsers is in the encoding of
//' non-ASCII characters. All non-ASCII characters will be escaped byte-by-byte.
//' If conformant non-ASCII behavior is important, ensure that your input vector
//' is UTF-8 encoded before calling encodeURI or encodeURIComponent.
//' 
//' @param value Character vector to be encoded or decoded.
//' @return Encoded or decoded character vector of the same length as the
//'   input value.
//'
//' @export
// [[Rcpp::export]]
std::vector<std::string> encodeURI(std::vector<std::string> value) {
  for (std::vector<std::string>::iterator it = value.begin();
    it != value.end();
    it++) {

    *it = doEncodeURI(*it, false);
  }
  
  return value;
}

//' @rdname encodeURI
//' @export
// [[Rcpp::export]]
std::vector<std::string> encodeURIComponent(std::vector<std::string> value) {
  for (std::vector<std::string>::iterator it = value.begin();
    it != value.end();
    it++) {

    *it = doEncodeURI(*it, true);
  }
  
  return value;
}

int hexToInt(char c) {
  switch (c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': case 'a': return 10;
    case 'B': case 'b': return 11;
    case 'C': case 'c': return 12;
    case 'D': case 'd': return 13;
    case 'E': case 'e': return 14;
    case 'F': case 'f': return 15;
    default: return -1;
  }
}

std::string doDecodeURI(std::string value, bool component) {
  std::ostringstream os;
  for (std::string::const_iterator it = value.begin();
    it != value.end();
    it++) {
    
    // If there aren't enough characters left for this to be a
    // valid escape code, just use the character and move on
    if (it > value.end() - 3) {
      os << *it;
      continue;
    }
    
    if (*it == '%') {
      char hi = *(++it);
      char lo = *(++it);
      int iHi = hexToInt(hi);
      int iLo = hexToInt(lo);
      if (iHi < 0 || iLo < 0) {
        // Invalid escape sequence
        os << '%' << hi << lo;
        continue;
      }
      char c = (char)(iHi << 4 | iLo);
      if (!component && isReservedUrlChar(c)) {
        os << '%' << hi << lo;
      } else {
        os << c;
      }
    } else {
      os << *it;
    }
  }
  
  return os.str();
}

//' @rdname encodeURI
//' @export
// [[Rcpp::export]]
std::vector<std::string> decodeURI(std::vector<std::string> value) {
  for (std::vector<std::string>::iterator it = value.begin();
    it != value.end();
    it++) {

    *it = doDecodeURI(*it, false);
  }
  
  return value;
}

//' @rdname encodeURI
//' @export
// [[Rcpp::export]]
std::vector<std::string> decodeURIComponent(std::vector<std::string> value) {
  for (std::vector<std::string>::iterator it = value.begin();
    it != value.end();
    it++) {

    *it = doDecodeURI(*it, true);
  }
  
  return value;
}

//' Check whether an address is IPv4 or IPv6
//'
//' Given an IP address, this checks whether it is an IPv4 or IPv6 address.
//'
//' @param ip A single string representing an IP address.
//'
//' @return
//' For IPv4 addresses, \code{4}; for IPv6 addresses, \code{6}. If the address is
//' neither, \code{-1}.
//'
//' @examples
//' ipFamily("127.0.0.1")   # 4
//' ipFamily("500.0.0.500") # -1
//' ipFamily("500.0.0.500") # -1
//'
//' ipFamily("::")          # 6
//' ipFamily("::1")         # 6
//' ipFamily("fe80::1ff:fe23:4567:890a") # 6
//' @export
// [[Rcpp::export]]
int ipFamily(const std::string& ip) {
  int family = ip_family(ip);
  if (family == AF_INET6)
    return 6;
  else if (family == AF_INET)
    return 4;
  else
    return -1;
}


// Given a List and an external pointer to a C++ function that takes a List,
// invoke the function with the List as the single argument. This also clears
// the external pointer so that the C++ function can't be called again.
// [[Rcpp::export]]
void invokeCppCallback(Rcpp::List data, SEXP callback_xptr) {
  ASSERT_MAIN_THREAD()

  if (TYPEOF(callback_xptr) != EXTPTRSXP) {
     throw Rcpp::exception("Expected external pointer.");
  }
  boost::function<void(Rcpp::List)>* callback_wrapper =
    (boost::function<void(Rcpp::List)>*)(R_ExternalPtrAddr(callback_xptr));

  (*callback_wrapper)(data);

  // We want to clear the external pointer to make sure that the C++ function
  // can't get called again by accident. Also delete the heap-allocated
  // boost::function.
  delete callback_wrapper;
  R_ClearExternalPtr(callback_xptr);
}

//' Apply the value of .Random.seed to R's internal RNG state
//'
//' This function is needed in unusual cases where a C++ function calls
//' an R function which sets the value of \code{.Random.seed}. This function
//' should be called at the end of the R function to ensure that the new value
//' \code{.Random.seed} is preserved. Otherwise, Rcpp may overwrite it with a
//' previous value.
//'
//' @keywords internal
//' @export
// [[Rcpp::export]]
void getRNGState() {
  GetRNGstate();
}

// We are given an external pointer to a
// boost::shared_ptr<WebSocketConnection>. This returns a hexadecimal string
// representing the address of the WebSocketConnection (not the shared_ptr to
// it!).
//
//[[Rcpp::export]]
std::string wsconn_address(SEXP external_ptr) {
  Rcpp::XPtr<boost::shared_ptr<WebSocketConnection> > xptr(external_ptr);
  std::ostringstream os;
  os << std::hex << reinterpret_cast<uintptr_t>(xptr.get()->get());
  return os.str();
}
