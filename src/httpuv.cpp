#include <Rcpp.h>
#include <stdio.h>
#include <map>
#include <string>
#include <Rinternals.h>
#undef Realloc
// Also need to undefine the Free macro
#undef Free
#include <uv.h>
#include "uvutil.h"
#include "http.h"

// TODO: Re. R_ignore_SIGPIPE... there must be a better way!?

#ifndef _WIN32
extern int R_ignore_SIGPIPE;
#else
static int R_ignore_SIGPIPE;
#endif

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

class RWebApplication : public WebApplication {
private:
  Rcpp::Function _onRequest;
  Rcpp::Function _onWSOpen;
  Rcpp::Function _onWSMessage;
  Rcpp::Function _onWSClose;

public:
  RWebApplication(Rcpp::Function onRequest, Rcpp::Function onWSOpen,
                  Rcpp::Function onWSMessage, Rcpp::Function onWSClose) :
    _onRequest(onRequest), _onWSOpen(onWSOpen), _onWSMessage(onWSMessage),
    _onWSClose(onWSClose) {

  }

  virtual ~RWebApplication() {
  }

  virtual HttpResponse* getResponse(HttpRequest* pRequest) {
    using namespace Rcpp;

    std::string url = pRequest->url();
    size_t qsIndex = url.find('?');
    std::string path, queryString;
    if (qsIndex == std::string::npos)
      path = url;
    else {
      path = url.substr(0, qsIndex);
      queryString = url.substr(qsIndex);
    }

    Environment env = Rcpp::Function("new.env")();
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

    std::vector<char> body = pRequest->body();
    RawVector input = RawVector(body.size());
    std::copy(body.begin(), body.end(), input.begin());
    env["httpuv.body"] = input;

    std::map<std::string, std::string, compare_ci> headers = pRequest->headers();
    for (std::map<std::string, std::string>::iterator it = headers.begin();
      it != headers.end();
      it++) {
      env["HTTP_" + normalizeHeaderName(it->first)] = it->second;
    }

    
    R_ignore_SIGPIPE = 0;
    List response(_onRequest(env));
    R_ignore_SIGPIPE = 1;
    
    int status = Rcpp::as<int>(response["status"]);
    std::string statusDesc = getStatusDescription(status);
    
    List responseHeaders = response["headers"];
    
    RawVector responseBytes;
    if (Rf_isString(response["body"]))
      responseBytes = Function("charToRaw")(response["body"]);
    else
      responseBytes = response["body"];
    // Unnecessary copy
    std::vector<char> resp(responseBytes.begin(), responseBytes.end());

    HttpResponse* pResp = new HttpResponse(pRequest, status, statusDesc, resp);
    CharacterVector headerNames = responseHeaders.names();
    for (R_len_t i = 0; i < responseHeaders.size(); i++) {
      pResp->addHeader(
        std::string((char*)headerNames[i], headerNames[i].size()),
        Rcpp::as<std::string>(responseHeaders[i]));
    }

    return pResp;
  }

  void onWSOpen(WebSocketConnection* pConn) {
    R_ignore_SIGPIPE = 0;
    _onWSOpen(externalize(pConn));
    R_ignore_SIGPIPE = 1;
  }

  void onWSMessage(WebSocketConnection* pConn, bool binary, const char* data, size_t len) {
    R_ignore_SIGPIPE = 0;
    if (binary)
      _onWSMessage(externalize(pConn), binary, std::vector<char>(data, data + len));
    else
      _onWSMessage(externalize(pConn), binary, std::string(data, len));
    R_ignore_SIGPIPE = 1;
  }
  
  void onWSClose(WebSocketConnection* pConn) {
    R_ignore_SIGPIPE = 0;
    _onWSClose(externalize(pConn));
    R_ignore_SIGPIPE = 1;
  }

};

// [[Rcpp::export]]
void sendWSMessage(std::string conn, bool binary, Rcpp::RObject message) {
  R_ignore_SIGPIPE = 1;
  WebSocketConnection* wsc = internalize<WebSocketConnection>(conn);
  if (binary) {
    Rcpp::RawVector raw = Rcpp::as<Rcpp::RawVector>(message);
    wsc->sendWSMessage(Binary, reinterpret_cast<char*>(&raw[0]), raw.size());
  } else {
    std::string str = Rcpp::as<std::string>(message);
    wsc->sendWSMessage(Text, str.c_str(), str.size());
  }
  R_ignore_SIGPIPE = 0;
}

// [[Rcpp::export]]
void closeWS(std::string conn) {
  R_ignore_SIGPIPE = 1;
  WebSocketConnection* wsc = internalize<WebSocketConnection>(conn);
  wsc->closeWS();
  R_ignore_SIGPIPE = 0;
}

void destroyServer(std::string handle);

// [[Rcpp::export]]
Rcpp::RObject makeServer(const std::string& host, int port,
                    Rcpp::Function onRequest, Rcpp::Function onWSOpen,
                    Rcpp::Function onWSMessage, Rcpp::Function onWSClose) {

  using namespace Rcpp;
  // Deleted when owning pHandler is deleted
  RWebApplication* pHandler = 
    new RWebApplication(onRequest, onWSOpen, onWSMessage, onWSClose);
  uv_tcp_t* pServer = createServer(
    uv_default_loop(), host.c_str(), port, (WebApplication*)pHandler);

  if (!pServer) {
    delete pHandler;
    return R_NilValue;
  }

  return Rcpp::wrap(externalize(pServer));
}

// [[Rcpp::export]]
void destroyServer(std::string handle) {
  uv_tcp_t* pServer = internalize<uv_tcp_t>(handle);
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

  R_ignore_SIGPIPE = 1;
  bool result = uv_run(uv_default_loop(), UV_RUN_ONCE);
  R_ignore_SIGPIPE = 0;

  return result;
}
