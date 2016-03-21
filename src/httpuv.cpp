#define _FILE_OFFSET_BITS 64
#include <Rcpp.h>
#include <stdio.h>
#include <map>
#include <string>
#include <iomanip>
#include <signal.h>
#include <errno.h>
#include <Rinternals.h>
#include "fixup.h"
#include <uv.h>
#include <base64.hpp>
#include "uvutil.h"
#include "http.h"
#include "filedatasource.h"

std::string normalizeHeaderName(const std::string& name) {
  std::string result = name;
  for (std::string::iterator it = result.begin();
    it != result.end();
    it++) {
    if (*it == '-')
      *it = '_';
    else if (*it >= 'a' && *it <= 'z')
      *it = *it + ('A' - 'a');
  }
  return result;
}

const std::string& getStatusDescription(int code) {
  static std::map<int, std::string> statusDescs;
  static std::string unknown("Dunno");
  if (statusDescs.size() == 0) {
    statusDescs[100] = "Continue";
    statusDescs[101] = "Switching Protocols";
    statusDescs[200] = "OK";
    statusDescs[201] = "Created";
    statusDescs[202] = "Accepted";
    statusDescs[203] = "Non-Authoritative Information";
    statusDescs[204] = "No Content";
    statusDescs[205] = "Reset Content";
    statusDescs[206] = "Partial Content";
    statusDescs[300] = "Multiple Choices";
    statusDescs[301] = "Moved Permanently";
    statusDescs[302] = "Found";
    statusDescs[303] = "See Other";
    statusDescs[304] = "Not Modified";
    statusDescs[305] = "Use Proxy";
    statusDescs[307] = "Temporary Redirect";
    statusDescs[400] = "Bad Request";
    statusDescs[401] = "Unauthorized";
    statusDescs[402] = "Payment Required";
    statusDescs[403] = "Forbidden";
    statusDescs[404] = "Not Found";
    statusDescs[405] = "Method Not Allowed";
    statusDescs[406] = "Not Acceptable";
    statusDescs[407] = "Proxy Authentication Required";
    statusDescs[408] = "Request Timeout";
    statusDescs[409] = "Conflict";
    statusDescs[410] = "Gone";
    statusDescs[411] = "Length Required";
    statusDescs[412] = "Precondition Failed";
    statusDescs[413] = "Request Entity Too Large";
    statusDescs[414] = "Request-URI Too Long";
    statusDescs[415] = "Unsupported Media Type";
    statusDescs[416] = "Requested Range Not Satisifable";
    statusDescs[417] = "Expectation Failed";
    statusDescs[500] = "Internal Server Error";
    statusDescs[501] = "Not Implemented";
    statusDescs[502] = "Bad Gateway";
    statusDescs[503] = "Service Unavailable";
    statusDescs[504] = "Gateway Timeout";
    statusDescs[505] = "HTTP Version Not Supported";
  }
  std::map<int, std::string>::iterator it = statusDescs.find(code);
  if (it != statusDescs.end())
    return it->second;
  else
    return unknown;
}

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

void requestToEnv(HttpRequest* pRequest, Rcpp::Environment* pEnv) {
  using namespace Rcpp;

  Environment& env = *pEnv;

  std::string url = pRequest->url();
  size_t qsIndex = url.find('?');
  std::string path, queryString;
  if (qsIndex == std::string::npos)
    path = url;
  else {
    path = url.substr(0, qsIndex);
    queryString = url.substr(qsIndex);
  }

  env["REQUEST_METHOD"] = pRequest->method();
  env["SCRIPT_NAME"] = std::string("");
  env["PATH_INFO"] = path;
  env["QUERY_STRING"] = queryString;

  env["rook.version"] = "1.1-0";
  env["rook.url_scheme"] = "http";

  Address addr = pRequest->serverAddress();
  env["SERVER_NAME"] = addr.host;
  std::ostringstream portstr;
  portstr << addr.port;
  env["SERVER_PORT"] = portstr.str();

  Address raddr = pRequest->clientAddress();
  env["REMOTE_ADDR"] = raddr.host;
  std::ostringstream rportstr;
  rportstr << raddr.port;
  env["REMOTE_PORT"] = rportstr.str();

  const RequestHeaders& headers = pRequest->headers();
  for (RequestHeaders::const_iterator it = headers.begin();
    it != headers.end();
    it++) {
    env["HTTP_" + normalizeHeaderName(it->first)] = it->second;
  }
}

class RawVectorDataSource : public DataSource {
  Rcpp::RawVector _vector;
  R_len_t _pos;

public:
  RawVectorDataSource(const Rcpp::RawVector& vector) : _vector(vector), _pos(0) {
  }

  uint64_t size() const {
    return _vector.size();
  }

  uv_buf_t getData(size_t bytesDesired) {
    size_t bytes = _vector.size() - _pos;

    // Are we at the end?
    if (bytes == 0)
      return uv_buf_init(NULL, 0);

    if (bytesDesired < bytes)
      bytes = bytesDesired;
    char* buf = (char*)malloc(bytes);
    if (!buf) {
      throw Rcpp::exception("Couldn't allocate buffer");
    }

    for (size_t i = 0; i < bytes; i++) {
      buf[i] = _vector[_pos + i];
    }

    _pos += bytes;

    return uv_buf_init(buf, bytes);
  }

  void freeData(uv_buf_t buffer) {
    free(buffer.base);
  }

  void close() {
    delete this;
  }
};

HttpResponse* listToResponse(HttpRequest* pRequest,
                             const Rcpp::List& response) {
  using namespace Rcpp;

  if (response.isNULL() || response.size() == 0)
    return NULL;

  CharacterVector names = response.names();

  int status = Rcpp::as<int>(response["status"]);
  std::string statusDesc = getStatusDescription(status);

  List responseHeaders = response["headers"];

  // Self-frees when response is written
  DataSource* pDataSource = NULL;

  // The response can either contain:
  // - bodyFile: String value that names the file that should be streamed
  // - body: Character vector (which is charToRaw-ed) or raw vector, or NULL
    if (std::find(names.begin(), names.end(), "bodyFile") != names.end()) {
    FileDataSource* pFDS = new FileDataSource();
    pFDS->initialize(Rcpp::as<std::string>(response["bodyFile"]),
        Rcpp::as<bool>(response["bodyFileOwned"]));
    pDataSource = pFDS;
  }
  else if (Rf_isString(response["body"])) {
    RawVector responseBytes = Function("charToRaw")(response["body"]);
    pDataSource = new RawVectorDataSource(responseBytes);
  }
  else {
    RawVector responseBytes = response["body"];
    pDataSource = new RawVectorDataSource(responseBytes);
  }

  HttpResponse* pResp = new HttpResponse(pRequest, status, statusDesc,
                                         pDataSource);
  CharacterVector headerNames = responseHeaders.names();
  for (R_len_t i = 0; i < responseHeaders.size(); i++) {
    pResp->addHeader(
      std::string((char*)headerNames[i], headerNames[i].size()),
      Rcpp::as<std::string>(responseHeaders[i]));
  }

  return pResp;
}

class RWebApplication : public WebApplication {
private:
  Rcpp::Function _onHeaders;
  Rcpp::Function _onBodyData;
  Rcpp::Function _onRequest;
  Rcpp::Function _onWSOpen;
  Rcpp::Function _onWSMessage;
  Rcpp::Function _onWSClose;

public:
  RWebApplication(Rcpp::Function onHeaders,
                  Rcpp::Function onBodyData,
                  Rcpp::Function onRequest,
                  Rcpp::Function onWSOpen,
                  Rcpp::Function onWSMessage,
                  Rcpp::Function onWSClose) :
    _onHeaders(onHeaders), _onBodyData(onBodyData), _onRequest(onRequest),
    _onWSOpen(onWSOpen), _onWSMessage(onWSMessage), _onWSClose(onWSClose) {
  }

  virtual ~RWebApplication() {
  }

  virtual HttpResponse* onHeaders(HttpRequest* pRequest) {
    if (_onHeaders.isNULL()) {
      return NULL;
    }

    requestToEnv(pRequest, &pRequest->env());

    Rcpp::List response(_onHeaders(pRequest->env()));

    return listToResponse(pRequest, response);
  }

  virtual void onBodyData(HttpRequest* pRequest,
                          const char* pData, size_t length) {
    Rcpp::RawVector rawVector(length);
    std::copy(pData, pData + length, rawVector.begin());
    _onBodyData(pRequest->env(), rawVector);
  }

  virtual HttpResponse* getResponse(HttpRequest* pRequest) {
    Rcpp::List response(_onRequest(pRequest->env()));

    return listToResponse(pRequest, response);
  }

  void onWSOpen(HttpRequest* pRequest) {
    requestToEnv(pRequest, &pRequest->env());
    _onWSOpen(externalize<WebSocketConnection>(pRequest->websocket()), pRequest->env());
  }

  void onWSMessage(WebSocketConnection* pConn, bool binary, const char* data, size_t len) {
    if (binary)
      _onWSMessage(externalize<WebSocketConnection>(pConn), binary, std::vector<uint8_t>(data, data + len));
    else
      _onWSMessage(externalize<WebSocketConnection>(pConn), binary, std::string(data, len));
  }

  void onWSClose(WebSocketConnection* pConn) {
    _onWSClose(externalize<WebSocketConnection>(pConn));
  }

};

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
    uv_default_loop(), host.c_str(), port, (WebApplication*)pHandler);

  if (!pServer) {
    return R_NilValue;
  }

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
    uv_default_loop(), name.c_str(), mask, (WebApplication*)pHandler);

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

void stop_loop_timer_cb(uv_timer_t* handle, int status) {
  uv_stop(handle->loop);
}

// Run the libuv default loop for roughly timeoutMillis, then stop
// [[Rcpp::export]]
bool run(uint32_t timeoutMillis) {
  static uv_timer_t timer_req = {0};
  int r;

  if (!timer_req.loop) {
    r = uv_timer_init(uv_default_loop(), &timer_req);
    if (r) {
      throwLastError(uv_default_loop(),
          "Failed to initialize libuv timeout timer: ");
    }
  }

  if (timeoutMillis > 0) {
    uv_timer_stop(&timer_req);
    r = uv_timer_start(&timer_req, &stop_loop_timer_cb, timeoutMillis, 0);
    if (r) {
      throwLastError(uv_default_loop(),
          "Failed to start libuv timeout timer: ");
    }
  }

  // Must ignore SIGPIPE when libuv code is running, otherwise unexpectedly
  // closing connections kill us
#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);
#endif
  return uv_run(uv_default_loop(), UV_RUN_ONCE);
}

// [[Rcpp::export]]
void stopLoop() {
  uv_stop(uv_default_loop());
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
 * exposed by the uv_default_loop to trigger uv_run whenever necessary. It uses the non-blocking version
 * of uv_run (UV_RUN_NOWAIT).
 *
 * On Windows: creates a thread that initiates a call to run the libuv default loop using non-blocking version of uv_run (UV_RUN_NOWAIT).
 * It uses the Rhttpd (and s-u's background package) mechanism to enforce synchronization using a dummy message
 * window so that the libuv loop is run in the main thread not on the newly created thread via a blocking 'SendMessage'
 * call. 
 * 
 */

// change this to #define DBG(X) X to print debug messages
#define DBG(X)

#ifndef WIN32
#include <R_ext/eventloop.h>

#define UVSERVERACTIVITY 55
#define UVLOOPACTIVITY 57
#endif


#ifdef WIN32
#define WM_LIBUV_CALLBACK ( WM_USER + 1 )
#define THREAD_RUN 0x00
#define THREAD_DISPOSE 0x01

static HWND message_window;
static LRESULT CALLBACK LibuvWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#ifndef HWND_MESSAGE
#define HWND_MESSAGE ((HWND)-3)
#endif
#endif

class DaemonizedServer {
public:
  // the libuv object
  uv_stream_t *_pServer;
  
  // some bookeeping flags
  int needs_init;
  int in_process;
  
  #ifndef WIN32
  // events on the socket trigger libuv loop
  InputHandler *serverHandler;
  
  // events on the libuv app itself trigger the loop
  InputHandler *loopHandler;
  #else
  // the thread used in windows to initiate the libuv loop run
  HANDLE thread;
  char thread_status;
  #endif
  
  DaemonizedServer(uv_stream_t *pServer)
  : _pServer(pServer), needs_init(1), in_process(0) {}

  #ifdef WIN32
  // stops thread running triggering libuv loop run
  void stop_thread() {
    DBG(Rprintf("called stop_thread\n"));
    if (!thread) return;
    
    // signal thread exit
    DWORD ts = 0;
    int ntimes = 10;
    int i = 0;
    
    while (i < ntimes && GetExitCodeThread(thread, &ts) && ts == STILL_ACTIVE) {
      DBG(Rprintf("setting flag\n"));
      thread_status |= THREAD_DISPOSE;
      Sleep(1);
      i++;
    }
    
    DBG(Rprintf("left the loop, i=%d\n", i));
    // check if we looped out without exiting thread
    // and terminate from here (this is not good, but it'here as a stopgap)
    // have not seen it called in testing
    if (GetExitCodeThread(thread, &ts) && ts == STILL_ACTIVE) {
      DBG(Rprintf("have to do this the hard way\n"));
      TerminateThread(thread, 0);
    }
    thread = 0;  
  }
  #endif
  
  ~DaemonizedServer() {
    #ifndef WIN32
    if (loopHandler) {
      removeInputHandler(&R_InputHandlers, loopHandler);
    }
    
    if (serverHandler) {
      removeInputHandler(&R_InputHandlers, serverHandler);
    }
    #else 
    DBG(Rprintf("Destructor: calling stop_thread\n"));
    stop_thread();
    #endif
    
    DBG(Rprintf("Destructor: about to call freeServer\n"));
    if (_pServer) {
      freeServer(_pServer);
    }
    _pServer = NULL;
  }
  
  void setup(){
#ifdef WIN32
    DBG(Rprintf("setting up message window\n"));
    HINSTANCE instance = GetModuleHandle(NULL);
    LPCTSTR window_class = "libuv";
    WNDCLASS wndclass = { 0, LibuvWindowProc, 0, 0, instance, NULL, 0, 0, NULL, window_class };
    RegisterClass(&wndclass);
    message_window = CreateWindow(window_class, "libuv", 0, 1, 1, 1, 1, HWND_MESSAGE, NULL, instance, NULL);
    DBG(Rprintf("done creating message window\n"));
    
    // initialize thread status flag
    thread_status = THREAD_RUN;
#endif
    needs_init = 0;
  };
};

#ifdef WIN32
// this is the windows trickery used to make libuv run on the main thread
// while having a thread that makes calls to the libuv loop 
static void run_libuv_main_thread(DaemonizedServer *dServer);

// run_libuv is the name of the function called by input handlers in POSIX systems
// and by the new thread in windows. Here the function is defined to send a message
// to the dummy message window that will actually run libuv
static void run_libuv(DaemonizedServer *dServer)
{
  DBG(Rprintf("sending message to window\n"));
  SendMessage(message_window, WM_LIBUV_CALLBACK, 0, (LPARAM) dServer);
  DBG(Rprintf("sent message to window\n"));
}

// in the following, calls to run_libuv in win32 are rewritten to calls
// to run_libuv_main_thread
#define run_libuv run_libuv_main_thread
#endif

// this is function that actually runs the libuv loop
static void run_libuv_(void *ptr)
{
  // this fake loop is here to force
  // processing events
  // deals with strange behavior in some Ubuntu installations
  DBG(Rprintf("ready to run libuv default loop\n"));
  for (int i=0; i < 5; ++i) {
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
  }
  DBG(Rprintf("done running libuv default loop\n"));
}

// the function called (eventually) by the input handler
// in win32 this definition is rewritten by the preprocessor
// to define run_libuv_main_thread
static void run_libuv(DaemonizedServer *dServer)
{
  DBG(Rprintf("enterd run_libuv"));
  // only let it do it's thing one at a time
  if (dServer->in_process) return;
  
  // call the function above
  dServer->in_process = 1;
  run_libuv_((void *) dServer);
  dServer->in_process = 0;
  
  DBG(Rprintf("exiting run_libuv"));
}

#ifdef WIN32
// remove the win32 rewrite rule
#undef run_libuv
#endif

static void libuv_input_handler(void *data);

#ifdef WIN32
// this is triggered by the SendMessage call in run_libuv
static LRESULT CALLBACK LibuvWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DBG(Rprintf("entered LibuvWindowProc\n"));
  if (hwnd == message_window && uMsg == WM_LIBUV_CALLBACK) {
    DaemonizedServer *dServer = (DaemonizedServer *) lParam;
    // this was defined above using the rewrite rule 
    // to be the same function called by the inputHandlers in POSIX
    run_libuv_main_thread(dServer);
    return 0;
  }
  DBG(Rprintf("exiting LibuvWindowProc\n"));
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// this infinite loop runs on a new thread, it essentially makes the
// SendMessage call over and over
static DWORD WINAPI LibuvThreadProc(LPVOID lpParameter)
{
  DaemonizedServer *dServer = (DaemonizedServer *) lpParameter;
  if (!dServer) return 0;
  
  // run the libuv loop as long as thread exit hasn't been triggered
  while ((dServer->thread_status & THREAD_DISPOSE) == 0) {
    DBG(Rprintf("calling run_libuv from LibuvThreadProc\n"));
    run_libuv(dServer);
    DBG(Rprintf("done run_libuv from LibuvThreadProc\n"));
    Sleep(1);
  }
  DBG(Rprintf("exited server loop\n"));
  // returning from this function will exit the thread
  return 0;
}
#endif

// this may not be needed...
static void libuv_input_handler(void *data)
{
  run_libuv((DaemonizedServer *) data);
}

// [[Rcpp::export]]
Rcpp::RObject daemonize(std::string handle) {
  uv_stream_t *pServer = internalize<uv_stream_t >(handle);
  DaemonizedServer *dServer = new DaemonizedServer(pServer);
  
  // do initial setup, in WIN32 it creates the message-only window,
  // in POSIX it doesn't do anything
  if (dServer->needs_init) { dServer->setup(); }

#ifndef WIN32
  // this input handler deals with events on the app socket  
  // in libuv v1.x this should be replaced by uv_fileno()
  int fd = pServer->io_watcher.fd;
  dServer->serverHandler = addInputHandler(R_InputHandlers, fd, &libuv_input_handler, UVSERVERACTIVITY);
  if (dServer->serverHandler) dServer->serverHandler->userData = dServer;

  // this input handler deals with events triggered by the libuv loop itself
  fd = uv_backend_fd(uv_default_loop());
  dServer->loopHandler = addInputHandler(R_InputHandlers, fd, &libuv_input_handler, UVLOOPACTIVITY);
  if (dServer->loopHandler) dServer->loopHandler->userData = dServer;
#else
  // in WIN32 make a thread that calls libuv loop on a loop
  if (dServer->thread) {
    dServer->stop_thread();
  }
  dServer->thread = CreateThread(NULL, 0, LibuvThreadProc, (LPVOID) dServer, 0, 0);
#endif

  return Rcpp::wrap(externalize(dServer));
}

// [[Rcpp::export]]
void destroyDaemonizedServer(std::string handle) {
  DaemonizedServer *dServer = internalize<DaemonizedServer >(handle);
  if (dServer) {
    #ifdef WIN32
    if (dServer->thread) {
      DBG(Rprintf("Thread is not 0;"));
    }
    #endif
    
    DBG(Rprintf("deleting daemonized server"));
    delete dServer;
  }
}

//// END OF DAEMONIZATION CODE

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
      os << '%' << std::setw(2) << (int)*it;
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
