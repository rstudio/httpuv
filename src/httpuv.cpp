#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <map>
#include <iomanip>
#include <signal.h>
#include <errno.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <uv.h>
#include <base64.hpp>
#include "uvutil.h"
#include "webapplication.h"
#include "http.h"
#include "queue.h"
#include "guard.h"
#include <Rinternals.h>


uv_thread_t *io_thread_id = NULL;
uv_async_t async_writer;
queue< boost::function<void (void)> > write_queue;

// The uv loop that we'll use. Should be accessed via get_io_loop().
uv_loop_t io_loop;
bool io_loop_initialized = false;
uv_loop_t* get_io_loop() {
  // TODO: Use mutex here
  if (!io_loop_initialized) {
    uv_loop_init(&io_loop);
    io_loop_initialized = true;
  }

  return &io_loop;
}


// [[Rcpp::export]]
void sendWSMessage(std::string conn, bool binary, Rcpp::RObject message) {
  WebSocketConnection* wsc = internalize<WebSocketConnection>(conn);
  if (binary) {
    Rcpp::RawVector raw = Rcpp::as<Rcpp::RawVector>(message);
    wsc->sendWSMessage(Binary, reinterpret_cast<char*>(&raw[0]), raw.size());
  } else {
    std::string str = Rcpp::as<std::string>(message);
    wsc->sendWSMessage(Text, str.c_str(), str.size());
  }
}

// [[Rcpp::export]]
void closeWS(std::string conn) {
  WebSocketConnection* wsc = internalize<WebSocketConnection>(conn);
  wsc->closeWS();
}

void destroyServer(std::string handle);

// [[Rcpp::export]]
Rcpp::RObject makeTcpServer(const std::string& host, int port,
                            Rcpp::Function onHeaders,
                            Rcpp::Function onBodyData,
                            Rcpp::Function onRequest,
                            Rcpp::Function onWSOpen,
                            Rcpp::Function onWSMessage,
                            Rcpp::Function onWSClose) {

  using namespace Rcpp;
  // Deleted when owning pServer is deleted. If pServer creation fails,
  // it's still createTcpServer's responsibility to delete pHandler.
  RWebApplication* pHandler =
    new RWebApplication(onHeaders, onBodyData, onRequest, onWSOpen,
                        onWSMessage, onWSClose);
  uv_stream_t* pServer = createTcpServer(
    get_io_loop(), host.c_str(), port, (WebApplication*)pHandler);

  if (!pServer) {
    return R_NilValue;
  }
  
  return Rcpp::wrap(externalize<uv_stream_t>(pServer));
}

void flush_write_queue(uv_async_t *handle) {
  boost::function<void (void)> cb;

  while (1) {
    // Do queue operations inside this guarded scope, but we'll execute the
    // callback outside of the scope, since it doesn't need to be protected,
    // and this will make it possible for the other thread to do queue
    // operations while we're invoking the callback.
    {
      guard guard(write_queue.mutex);
      if (write_queue.size() == 0) {
        break;
      }

      cb = write_queue.front();
      write_queue.pop();
    }

    cb();
  }
}

void io_thread(void* data) {
  write_queue = queue< boost::function<void (void)> >();

  // uv_stream_t* pServer = static_cast<uv_stream_t*>(data);

  // Set up async communcation channels
  uv_async_init(get_io_loop(), &async_writer, flush_write_queue);

  uv_run(get_io_loop(), UV_RUN_DEFAULT);

  // TODO: Clean up pServer and other stuff?
}

// [[Rcpp::export]]
Rcpp::RObject makeBackgroundTcpServer(const std::string& host, int port,
                            Rcpp::Function onHeaders,
                            Rcpp::Function onBodyData,
                            Rcpp::Function onRequest,
                            Rcpp::Function onWSOpen,
                            Rcpp::Function onWSMessage,
                            Rcpp::Function onWSClose) {

  using namespace Rcpp;
  // Deleted when owning pServer is deleted. If pServer creation fails,
  // it's still createTcpServer's responsibility to delete pHandler.
  RWebApplication* pHandler =
    new RWebApplication(onHeaders, onBodyData, onRequest, onWSOpen,
                        onWSMessage, onWSClose);

  uv_stream_t* pServer = createTcpServer(
    get_io_loop(), host.c_str(), port, (WebApplication*)pHandler
  );

  if (!pServer) {
    return R_NilValue;
  }

  io_thread_id = (uv_thread_t *) malloc(sizeof(uv_thread_t));

  int ret = uv_thread_create(io_thread_id, io_thread, pServer);

  if (ret != 0) {
    // TODO: free memory
    Rcpp::stop(std::string("Error: ") + uv_strerror(ret));
  }
  // TODO: Check for error value

  // Return thread id instead?
  return Rcpp::wrap(externalize<uv_stream_t>(pServer));
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
  // Deleted when owning pServer is deleted. If pServer creation fails,
  // it's still createTcpServer's responsibility to delete pHandler.
  RWebApplication* pHandler =
    new RWebApplication(onHeaders, onBodyData, onRequest, onWSOpen,
                        onWSMessage, onWSClose);
  uv_stream_t* pServer = createPipeServer(
    get_io_loop(), name.c_str(), mask, (WebApplication*)pHandler);

  if (!pServer) {
    return R_NilValue;
  }

  return Rcpp::wrap(externalize(pServer));
}

// [[Rcpp::export]]
void destroyServer(std::string handle) {
  uv_stream_t* pServer = internalize<uv_stream_t>(handle);
  freeServer(pServer);
}

void dummy_close_cb(uv_handle_t* handle) {
}

void stop_loop_timer_cb(uv_timer_t* handle) {
  uv_stop(handle->loop);
}

// Run the libuv default loop until an I/O event occurs, or for up to
// timeoutMillis, then stop.
// [[Rcpp::export]]
bool run(int timeoutMillis) {
  static uv_timer_t timer_req = {0};
  int r;

  if (!timer_req.loop) {
    r = uv_timer_init(get_io_loop(), &timer_req);
    if (r) {
      throwError(r,
          "Failed to initialize libuv timeout timer: ");
    }
  }

  if (timeoutMillis > 0) {
    uv_timer_stop(&timer_req);
    r = uv_timer_start(&timer_req, &stop_loop_timer_cb, timeoutMillis, 0);
    if (r) {
      throwError(r,
          "Failed to start libuv timeout timer: ");
    }
  }

  // Must ignore SIGPIPE when libuv code is running, otherwise unexpectedly
  // closing connections kill us
#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);
#endif
  return uv_run(get_io_loop(), timeoutMillis == NA_INTEGER ? UV_RUN_NOWAIT : UV_RUN_ONCE);
}

// [[Rcpp::export]]
void stopLoop() {
  uv_stop(get_io_loop());
}

// [[Rcpp::export]]
std::string base64encode(const Rcpp::RawVector& x) {
  return b64encode(x.begin(), x.end());
}

/*
 * Daemonizing
 * 
 * On UNIX-like environments: Uses the R event loop to trigger the libuv default loop. This is a similar mechanism as that used by Rhttpd.
 * It adds an event listener on the port where the TCP server was created by libuv. This triggers uv_run on the
 * default loop any time there is an event on the server port. It also adds an event listener to a file descriptor
 * exposed by the get_io_loop to trigger uv_run whenever necessary. It uses the non-blocking version
 * of uv_run (UV_RUN_NOWAIT).
 *
 * On Windows: creates a thread that runs the libuv default loop. It uses the usual "service" mechanism
 * on the new thread (it uses the run function defined above). TODO: check synchronization. 
 *
 */

#ifndef WIN32
#include <R_ext/eventloop.h>

#define UVSERVERACTIVITY 55
#define UVLOOPACTIVITY 57
#endif

void loop_input_handler(void *data) {
  #ifndef WIN32
  // this fake loop is here to force
  // processing events
  // deals with strange behavior in some Ubuntu installations
  for (int i=0; i < 5; ++i) {
    uv_run(get_io_loop(), UV_RUN_NOWAIT);
  }
  #else
  bool res = 1;
  while (res) {
    res = run(100);
    Sleep(1);
  }
  #endif
}

#ifdef WIN32
static DWORD WINAPI ServerThreadProc(LPVOID lpParameter) {
  loop_input_handler(lpParameter);
  return 0;
}
#endif

class DaemonizedServer {
public:
  uv_stream_t *_pServer;
  #ifndef WIN32
  InputHandler *serverHandler;
  InputHandler *loopHandler;
  #else
  HANDLE server_thread;
  #endif
  
  DaemonizedServer(uv_stream_t *pServer)
  : _pServer(pServer) {}

  ~DaemonizedServer() {
    #ifndef WIN32
    if (loopHandler) {
      removeInputHandler(&R_InputHandlers, loopHandler);
    }
    
    if (serverHandler) {
      removeInputHandler(&R_InputHandlers, serverHandler);
    }
    #else 
      if (server_thread) {
        DWORD ts = 0;
        if (GetExitCodeThread(server_thread, &ts) && ts == STILL_ACTIVE)
          TerminateThread(server_thread, 0);
        server_thread = 0;
      }
    #endif
    
    if (_pServer) {
      freeServer(_pServer);
    }
  }
  void setup(){
  };
};

// [[Rcpp::export]]
Rcpp::RObject daemonize(std::string handle) {
  uv_stream_t *pServer = internalize<uv_stream_t >(handle);
  DaemonizedServer *dServer = new DaemonizedServer(pServer);

   #ifndef WIN32
   int fd = pServer->io_watcher.fd;
   dServer->serverHandler = addInputHandler(R_InputHandlers, fd, &loop_input_handler, UVSERVERACTIVITY);

   fd = uv_backend_fd(get_io_loop());
   dServer->loopHandler = addInputHandler(R_InputHandlers, fd, &loop_input_handler, UVLOOPACTIVITY);
   #else
   if (dServer->server_thread) {
     DWORD ts = 0;
     if (GetExitCodeThread(dServer->server_thread, &ts) && ts == STILL_ACTIVE)
       TerminateThread(dServer->server_thread, 0);
     dServer->server_thread = 0;
   }
   dServer->server_thread = CreateThread(NULL, 0, ServerThreadProc, 0, 0, 0);
   #endif

  return Rcpp::wrap(externalize(dServer));
}

// [[Rcpp::export]]
void destroyDaemonizedServer(std::string handle) {
  DaemonizedServer *dServer = internalize<DaemonizedServer >(handle);
  delete dServer;
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

// Given a List and an external pointer to a C++ function that takes a List,
// invoke the function with the List as the single argument. This also clears
// the external pointer so that the C++ function can't be called again.
// [[Rcpp::export]]
void invokeCppCallback(Rcpp::List data, SEXP callback_sexp) {
  if (TYPEOF(callback_sexp) != EXTPTRSXP) {
     throw Rcpp::exception("Expected external pointer.");
  }
  Rcpp::XPtr< boost::function<void(Rcpp::List)> > callback_xptr(callback_sexp);
  boost::function<void(Rcpp::List)> callback = *callback_xptr;
  callback(data);

  // We want to clear the external pointer to make sure that the C++ function
  // can't get called again by accident. But if we do this, the Xptr's finalizer
  // won't work correctly because it'll be deleting a NULL pointer. So we have
  // to delete it explicitly before clearing the external pointer.
  //
  // Free the callback_wrapper allocated in onHeaders or getResponse.
  delete callback_xptr.get();
  R_ClearExternalPtr(callback_sexp);
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
