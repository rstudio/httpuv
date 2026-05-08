#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <map>
#include <iomanip>
#include <signal.h>
#include <errno.h>
#include <functional>
#include <memory>
#include <uv.h>
#include "base64/base64.hpp"
#include "uvutil.h"
#include "webapplication.h"
#include "http.h"
#include "callbackqueue.h"
#include "utils.h"
#include "thread.h"
#include "httpuv.h"
#include "auto_deleter.h"
#include "socket.h"
#include <Rinternals.h>
#ifdef length
# undef length
#endif
#include <R_ext/Random.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>


void throwError(int err,
  const std::string& prefix = std::string(),
  const std::string& suffix = std::string())
{
  ASSERT_MAIN_THREAD()
  std::string msg = prefix + uv_strerror(err) + suffix;
  throw std::runtime_error(msg.c_str());
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
  if (!uv_is_closing(handle)) {
    uv_close(handle, NULL);
  }
}

void stop_io_loop(uv_async_t *handle) {
  ASSERT_BACKGROUND_THREAD()
  debug_log("stop_io_loop", LOG_DEBUG);
  uv_stop(io_loop.get());
}

#ifndef _WIN32
// Blocks SIGPIPE on the current thread.
void block_sigpipe() {
  sigset_t set;
  int result;
  sigemptyset(&set);
  sigaddset(&set, SIGPIPE);
  result = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if (result) {
    err_printf("Error blocking SIGPIPE on httpuv background thread.\n");
  }
}
#endif

void io_thread(void* data) {
  register_background_thread();
  std::shared_ptr<Barrier>* pBlocker = reinterpret_cast<std::shared_ptr<Barrier>*>(data);
  std::shared_ptr<Barrier> blocker = std::shared_ptr<Barrier>(*pBlocker);
  delete pBlocker;

  io_thread_running.set(true);

  io_loop.ensure_initialized();

  background_queue = new CallbackQueue(io_loop.get());

  // Set up async communication channels
  uv_async_init(io_loop.get(), &async_stop_io_loop, stop_io_loop);

  // Tell other thread that it can continue.
  blocker->wait();

  // Must ignore SIGPIPE for libuv code; otherwise unexpectedly closed
  // connections kill us. https://github.com/rstudio/httpuv/issues/168
#ifndef _WIN32
  block_sigpipe();
#endif
  // Run io_loop. When it stops, this fuction continues and the thread exits.
  uv_run(io_loop.get(), UV_RUN_DEFAULT);

  debug_log("io_loop stopped", LOG_DEBUG);

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

  // Use a shared_ptr because the lifetime of this object might be longer than
  // this function, since it is passed to the background thread.
  std::shared_ptr<Barrier> blocker = std::make_shared<Barrier>(2);

  // We want to pass a copy of the shared_ptr to the new pthread. To do that, we
  // need to create a new shared_ptr and get the regular pointer to it.
  std::shared_ptr<Barrier>* pBlocker = new std::shared_ptr<Barrier>(blocker);

  int ret = uv_thread_create(&io_thread_id, io_thread, pBlocker);
  // Wait for io_loop to be initialized before continuing
  blocker->wait();

  if (ret != 0) {
    Rf_error("Error: %s", uv_strerror(ret));
  }
}


// ============================================================================
// Outgoing websocket messages
// ============================================================================

SEXP sendWSMessage(SEXP conn, SEXP binary, SEXP message)
{
  ASSERT_MAIN_THREAD()
  std::shared_ptr<WebSocketConnection> wsc = internalize_shared_ptr(conn);

  Opcode mode;
  SEXP msg_sexp;
  std::vector<char>* str;

  // Efficiently copy message into a new vector<char>.
  if (LOGICAL(binary)[0]) {
    mode = Binary;
    msg_sexp = PROTECT(message);
    str = new std::vector<char>(RAW(msg_sexp), RAW(msg_sexp) + Rf_length(msg_sexp));
    UNPROTECT(1);

  } else {
    mode = Text;
    msg_sexp = PROTECT(STRING_ELT(message, 0));
    str = new std::vector<char>(CHAR(msg_sexp), CHAR(msg_sexp) + Rf_length(msg_sexp));
    UNPROTECT(1);
  }

  std::function<void (void)> cb(
    std::bind(&WebSocketConnection::sendWSMessage, wsc,
      mode,
      safe_vec_addr(*str),
      str->size()
    )
  );

  background_queue->push(cb);
  background_queue->push(std::bind(deleter_background<std::vector<char> >, str));
  return R_NilValue;
}

SEXP closeWS(SEXP conn, SEXP code, SEXP reason)
{
  ASSERT_MAIN_THREAD()
  debug_log("closeWS", LOG_DEBUG);
  std::shared_ptr<WebSocketConnection> wsc = internalize_shared_ptr(conn);

  uint16_t code_val = (uint16_t)INTEGER(code)[0];
  std::string reason_str = std::string(CHAR(STRING_ELT(reason, 0)));

  background_queue->push(
    std::bind(&WebSocketConnection::closeWS, wsc, code_val, reason_str)
  );
  return R_NilValue;
}


// ============================================================================
// Create/stop servers
// ============================================================================

SEXP makeTcpServer(SEXP host, SEXP port,
                   SEXP onHeaders,
                   SEXP onBodyData,
                   SEXP onRequest,
                   SEXP onWSOpen,
                   SEXP onWSMessage,
                   SEXP onWSClose,
                   SEXP staticPaths,
                   SEXP staticPathOptions,
                   SEXP quiet)
{
  register_main_thread();

  std::string host_str = std::string(CHAR(STRING_ELT(host, 0)));
  int port_int = INTEGER(port)[0];
  bool quiet_bool = LOGICAL(quiet)[0] != 0;

  // Deleted when owning pServer is deleted. If pServer creation fails,
  // this should be deleted when it goes out of scope.
  std::shared_ptr<RWebApplication> pHandler(
    new RWebApplication(onHeaders, onBodyData, onRequest,
                        onWSOpen, onWSMessage, onWSClose,
                        staticPaths, staticPathOptions),
    auto_deleter_main<RWebApplication>
  );

  ensure_io_thread();

  std::shared_ptr<Barrier> blocker = std::make_shared<Barrier>(2);

  uv_stream_t* pServer;

  background_queue->push(
    std::bind(createTcpServerSync,
      io_loop.get(), host_str.c_str(), port_int,
      std::static_pointer_cast<WebApplication>(pHandler),
      quiet_bool, background_queue, &pServer, blocker
    )
  );

  // Wait for server to be created before continuing
  blocker->wait();

  if (!pServer) {
    return R_NilValue;
  }

  pServers.push_back(pServer);

  return Rf_mkString(externalize_str<uv_stream_t>(pServer).c_str());
}

SEXP makePipeServer(SEXP name, SEXP mask,
                    SEXP onHeaders,
                    SEXP onBodyData,
                    SEXP onRequest,
                    SEXP onWSOpen,
                    SEXP onWSMessage,
                    SEXP onWSClose,
                    SEXP staticPaths,
                    SEXP staticPathOptions,
                    SEXP quiet)
{
  register_main_thread();

  std::string name_str = std::string(CHAR(STRING_ELT(name, 0)));
  int mask_int = INTEGER(mask)[0];
  bool quiet_bool = LOGICAL(quiet)[0] != 0;

  // Deleted when owning pServer is deleted. If pServer creation fails,
  // this should be deleted when it goes out of scope.
  std::shared_ptr<RWebApplication> pHandler(
    new RWebApplication(onHeaders, onBodyData, onRequest,
                        onWSOpen, onWSMessage, onWSClose,
                        staticPaths, staticPathOptions),
    auto_deleter_main<RWebApplication>
  );

  ensure_io_thread();

  std::shared_ptr<Barrier> blocker = std::make_shared<Barrier>(2);

  uv_stream_t* pServer;

  background_queue->push(
    std::bind(createPipeServerSync,
      io_loop.get(), name_str.c_str(), mask_int,
      std::static_pointer_cast<WebApplication>(pHandler),
      quiet_bool, background_queue, &pServer, blocker
    )
  );

  // Wait for server to be created before continuing
  blocker->wait();

  if (!pServer) {
    return R_NilValue;
  }

  pServers.push_back(pServer);

  return Rf_mkString(externalize_str<uv_stream_t>(pServer).c_str());
}


void stopServer_(uv_stream_t* pServer) {
  ASSERT_MAIN_THREAD()

  // Remove it from the list of running servers.
  // Note: we're removing it from the pServers list without waiting for the
  // background thread to call freeServer().
  std::vector<uv_stream_t*>::iterator pos = std::find(pServers.begin(), pServers.end(), pServer);
  if (pos != pServers.end()) {
    pServers.erase(pos);
  } else {
    throw std::runtime_error("pServer handle not found in list of running servers.");
  }

  // Run on background thread:
  // freeServer(pServer);
  background_queue->push(
    std::bind(freeServer, pServer)
  );
}

SEXP stopServer_sexp(SEXP handle) {
  ASSERT_MAIN_THREAD()
  std::string handle_str = std::string(CHAR(STRING_ELT(handle, 0)));
  uv_stream_t* pServer = internalize_str<uv_stream_t>(handle_str);
  stopServer_(pServer);
  return R_NilValue;
}

void stop_loop_timer_cb(uv_timer_t* handle) {
  uv_stop(handle->loop);
}


// ============================================================================
// Static file serving
// ============================================================================

std::shared_ptr<WebApplication> get_pWebApplication(uv_stream_t* pServer) {
  // Copy the Socket shared_ptr
  std::shared_ptr<Socket> pSocket(*(std::shared_ptr<Socket>*)pServer->data);
  return pSocket->pWebApplication;
}

std::shared_ptr<WebApplication> get_pWebApplication(std::string handle) {
  uv_stream_t* pServer = internalize_str<uv_stream_t>(handle);
  return get_pWebApplication(pServer);
}

SEXP getStaticPaths_(SEXP handle) {
  ASSERT_MAIN_THREAD()
  std::string h = std::string(CHAR(STRING_ELT(handle, 0)));
  return get_pWebApplication(h)->getStaticPathManager().pathsAsRObject();
}

SEXP setStaticPaths_(SEXP handle, SEXP sp) {
  ASSERT_MAIN_THREAD()
  std::string h = std::string(CHAR(STRING_ELT(handle, 0)));
  get_pWebApplication(h)->getStaticPathManager().set(sp);
  return get_pWebApplication(h)->getStaticPathManager().pathsAsRObject();
}

SEXP removeStaticPaths_(SEXP handle, SEXP paths) {
  ASSERT_MAIN_THREAD()
  std::string h = std::string(CHAR(STRING_ELT(handle, 0)));
  get_pWebApplication(h)->getStaticPathManager().remove(paths);
  return get_pWebApplication(h)->getStaticPathManager().pathsAsRObject();
}

SEXP getStaticPathOptions_(SEXP handle) {
  ASSERT_MAIN_THREAD()
  std::string h = std::string(CHAR(STRING_ELT(handle, 0)));
  return get_pWebApplication(h)->getStaticPathManager().getOptions().asRObject();
}

SEXP setStaticPathOptions_(SEXP handle, SEXP opts) {
  ASSERT_MAIN_THREAD()
  std::string h = std::string(CHAR(STRING_ELT(handle, 0)));
  get_pWebApplication(h)->getStaticPathManager().setOptions(opts);
  return get_pWebApplication(h)->getStaticPathManager().getOptions().asRObject();
}


// ============================================================================
// Miscellaneous utility functions
// ============================================================================

SEXP base64encode(SEXP x) {
  const uint8_t* begin = RAW(x);
  const uint8_t* end = begin + Rf_xlength(x);
  std::string result = b64encode(begin, end);
  return Rf_mkString(result.c_str());
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
      os << '%' << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(*it));
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
//' Intended as a faster replacement for [utils::URLencode()] and
//' [utils::URLdecode()].
//'
//' encodeURI differs from encodeURIComponent in that the former will not encode
//' reserved characters: \code{;,/?:@@&=+$}
//'
//' decodeURI differs from decodeURIComponent in that it will refuse to decode
//' encoded sequences that decode to a reserved character. (If in doubt, use
//' decodeURIComponent.)
//'
//' For \code{encodeURI} and \code{encodeURIComponent}, input strings will be
//' converted to UTF-8 before URL-encoding.
//'
//' @param value Character vector to be encoded or decoded.
//' @return Encoded or decoded character vector of the same length as the
//'   input value. \code{decodeURI} and \code{decodeURIComponent} will return
//'   strings that are UTF-8 encoded.
//'
//' @export
SEXP encodeURI(SEXP value) {
  R_xlen_t n = Rf_xlength(value);
  SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
  for (R_xlen_t i = 0; i < n; i++) {
    SET_STRING_ELT(out, i, NA_STRING);
  }

  for (R_xlen_t i = 0; i < n; i++) {
    if (STRING_ELT(value, i) != NA_STRING) {
      std::string encoded = doEncodeURI(Rf_translateCharUTF8(STRING_ELT(value, i)), false);
      SET_STRING_ELT(out, i, Rf_mkCharCE(encoded.c_str(), CE_UTF8));
    }
  }
  UNPROTECT(1);
  return out;
}

//' @rdname encodeURI
//' @export
SEXP encodeURIComponent(SEXP value) {
  R_xlen_t n = Rf_xlength(value);
  SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
  for (R_xlen_t i = 0; i < n; i++) {
    SET_STRING_ELT(out, i, NA_STRING);
  }

  for (R_xlen_t i = 0; i < n; i++) {
    if (STRING_ELT(value, i) != NA_STRING) {
      std::string encoded = doEncodeURI(Rf_translateCharUTF8(STRING_ELT(value, i)), true);
      SET_STRING_ELT(out, i, Rf_mkCharCE(encoded.c_str(), CE_UTF8));
    }
  }
  UNPROTECT(1);
  return out;
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
SEXP decodeURI(SEXP value) {
  R_xlen_t n = Rf_xlength(value);
  SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
  for (R_xlen_t i = 0; i < n; i++) {
    SET_STRING_ELT(out, i, NA_STRING);
  }

  for (R_xlen_t i = 0; i < n; i++) {
    if (STRING_ELT(value, i) != NA_STRING) {
      std::string decoded = doDecodeURI(std::string(CHAR(STRING_ELT(value, i))), false);
      SET_STRING_ELT(out, i, Rf_mkCharLenCE(decoded.c_str(), (int)decoded.size(), CE_UTF8));
    }
  }

  UNPROTECT(1);
  return out;
}

//' @rdname encodeURI
//' @export
SEXP decodeURIComponent(SEXP value) {
  R_xlen_t n = Rf_xlength(value);
  SEXP out = PROTECT(Rf_allocVector(STRSXP, n));
  for (R_xlen_t i = 0; i < n; i++) {
    SET_STRING_ELT(out, i, NA_STRING);
  }

  for (R_xlen_t i = 0; i < n; i++) {
    if (STRING_ELT(value, i) != NA_STRING) {
      std::string decoded = doDecodeURI(std::string(CHAR(STRING_ELT(value, i))), true);
      SET_STRING_ELT(out, i, Rf_mkCharLenCE(decoded.c_str(), (int)decoded.size(), CE_UTF8));
    }
  }

  UNPROTECT(1);
  return out;
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
SEXP ipFamily(SEXP ip) {
  std::string ip_str = std::string(CHAR(STRING_ELT(ip, 0)));
  int family = ip_family(ip_str);
  if (family == AF_INET6)
    return Rf_ScalarInteger(6);
  else if (family == AF_INET)
    return Rf_ScalarInteger(4);
  else
    return Rf_ScalarInteger(-1);
}


// Given a List and an external pointer to a C++ function that takes a List,
// invoke the function with the List as the single argument. This also clears
// the external pointer so that the C++ function can't be called again.
SEXP invokeCppCallback(SEXP data, SEXP callback_xptr) {
  ASSERT_MAIN_THREAD()

  if (TYPEOF(callback_xptr) != EXTPTRSXP) {
     Rf_error("Expected external pointer.");
  }
  std::function<void(SEXP)>* callback_wrapper =
    (std::function<void(SEXP)>*)(R_ExternalPtrAddr(callback_xptr));

  (*callback_wrapper)(data);

  // We want to clear the external pointer to make sure that the C++ function
  // can't get called again by accident. Also delete the heap-allocated
  // std::function.
  delete callback_wrapper;
  R_ClearExternalPtr(callback_xptr);
  return R_NilValue;
}

//' Apply the value of .Random.seed to R's internal RNG state
//'
//' This function is needed in unusual cases where a C++ function calls
//' an R function which sets the value of \code{.Random.seed}. This function
//' should be called at the end of the R function to ensure that the new value
//' \code{.Random.seed} is preserved. Otherwise, the C++ code may overwrite it
//' with a previous value.
//'
//' @keywords internal
//' @export
SEXP getRNGState(void) {
  GetRNGstate();
  return R_NilValue;
}

// We are given an external pointer to a
// std::shared_ptr<WebSocketConnection>. This returns a hexadecimal string
// representing the address of the WebSocketConnection (not the shared_ptr to
// it!).
SEXP wsconn_address(SEXP external_ptr) {
  std::shared_ptr<WebSocketConnection>* xptr =
    (std::shared_ptr<WebSocketConnection>*)R_ExternalPtrAddr(external_ptr);
  std::ostringstream os;
  os << std::hex << reinterpret_cast<uintptr_t>(xptr->get());
  return Rf_mkString(os.str().c_str());
}

// ============================================================================
// R function registration
// ============================================================================

extern "C" {

static const R_CallMethodDef callMethods[] = {
  {"sendWSMessage",         (DL_FUNC)&sendWSMessage,         3},
  {"closeWS",               (DL_FUNC)&closeWS,               3},
  {"makeTcpServer",         (DL_FUNC)&makeTcpServer,        11},
  {"makePipeServer",        (DL_FUNC)&makePipeServer,       11},
  {"stopServer_",           (DL_FUNC)&stopServer_sexp,       1},
  {"getStaticPaths_",       (DL_FUNC)&getStaticPaths_,       1},
  {"setStaticPaths_",       (DL_FUNC)&setStaticPaths_,       2},
  {"removeStaticPaths_",    (DL_FUNC)&removeStaticPaths_,    2},
  {"getStaticPathOptions_", (DL_FUNC)&getStaticPathOptions_, 1},
  {"setStaticPathOptions_", (DL_FUNC)&setStaticPathOptions_, 2},
  {"base64encode",          (DL_FUNC)&base64encode,          1},
  {"encodeURI",             (DL_FUNC)&encodeURI,             1},
  {"encodeURIComponent",    (DL_FUNC)&encodeURIComponent,    1},
  {"decodeURI",             (DL_FUNC)&decodeURI,             1},
  {"decodeURIComponent",    (DL_FUNC)&decodeURIComponent,    1},
  {"ipFamily",              (DL_FUNC)&ipFamily,              1},
  {"invokeCppCallback",     (DL_FUNC)&invokeCppCallback,     2},
  {"getRNGState",           (DL_FUNC)&getRNGState,           0},
  {"wsconn_address",        (DL_FUNC)&wsconn_address,        1},
  {"log_level",             (DL_FUNC)&log_level,             1},
  {NULL, NULL, 0}
};

void attribute_visible R_init_httpuv(DllInfo *dll) {
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
}

} // extern "C"
