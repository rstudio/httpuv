#include <Rcpp.h>
#include <stdio.h>
#include <map>
#include <string>
#include <uv.h>
#include "http.hpp"

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

    env["rook.version"] = "0.0";
    env["rook.url_scheme"] = "http";

    std::vector<char> body = pRequest->body();
    RawVector input = RawVector(body.size());
    std::copy(body.begin(), body.end(), input.begin());
    env["rook.input"] = input;

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
    
    RawVector responseBytes = response["body"];
    // Unnecessary copy
    std::vector<char> resp(responseBytes.begin(), responseBytes.end());

    HttpResponse* pResp = new HttpResponse(pRequest, status, statusDesc, resp);
    CharacterVector headerNames = responseHeaders.names();
    for (size_t i = 0; i < responseHeaders.size(); i++) {
      pResp->addHeader(
        std::string((char*)headerNames[i], headerNames[i].size()),
        Rcpp::as<std::string>(responseHeaders[i]));
    }

    return pResp;
  }

  void onWSOpen(WebSocketConnection* pConn) {
    R_ignore_SIGPIPE = 0;
    _onWSOpen((intptr_t)pConn);
    R_ignore_SIGPIPE = 1;
  }

  void onWSMessage(WebSocketConnection* pConn, bool binary, const char* data, size_t len) {
    R_ignore_SIGPIPE = 0;
    if (binary)
      _onWSMessage((intptr_t)pConn, binary, std::vector<char>(data, data + len));
    else
      _onWSMessage((intptr_t)pConn, binary, std::string(data, len));
    R_ignore_SIGPIPE = 1;
  }
  
  void onWSClose(WebSocketConnection* pConn) {
    R_ignore_SIGPIPE = 0;
    _onWSClose((intptr_t)pConn);
    R_ignore_SIGPIPE = 1;
  }

};

// [[Rcpp::export]]
void sendWSMessage(intptr_t conn, bool binary, Rcpp::RObject message) {
  R_ignore_SIGPIPE = 1;
  WebSocketConnection* wsc = reinterpret_cast<WebSocketConnection*>(conn);
  if (binary) {
    Rcpp::RawVector raw = Rcpp::as<Rcpp::RawVector>(message);
    wsc->sendWSMessage(binary, reinterpret_cast<char*>(&raw[0]), raw.size());
  } else {
    std::string str = Rcpp::as<std::string>(message);
    wsc->sendWSMessage(binary, str.c_str(), str.size());
  }
  R_ignore_SIGPIPE = 0;
}

// [[Rcpp::export]]
void closeWS(intptr_t conn) {
  R_ignore_SIGPIPE = 1;
  std::cerr << "GOT HERE\n";
  WebSocketConnection* wsc = reinterpret_cast<WebSocketConnection*>(conn);
  wsc->closeWS();
  R_ignore_SIGPIPE = 0;
}

// [[Rcpp::export]]
intptr_t makeServer(const std::string& host, int port,
                    Rcpp::Function onRequest, Rcpp::Function onWSOpen,
                    Rcpp::Function onWSMessage, Rcpp::Function onWSClose) {

  using namespace Rcpp;
  // Deleted when owning pHandler is deleted
  // TODO: When is this deleted??
  RWebApplication* pHandler = 
    new RWebApplication(onRequest, onWSOpen, onWSMessage, onWSClose);
  uv_tcp_t* pServer = createServer(
    uv_default_loop(), host.c_str(), port, (WebApplication*)pHandler);

  std::cerr << "makeServer " << (intptr_t)pServer << "\n";
  return (intptr_t)pServer;
}

// [[Rcpp::export]]
void destroyServer(intptr_t handle) {
  std::cerr << "destroyServer " << handle << "\n";
  freeServer((uv_tcp_t*)handle);
}

// [[Rcpp::export]]
bool runNB() {
  void (*origHandler)(int);
  R_ignore_SIGPIPE = 1;
  bool result = runNonBlocking(uv_default_loop());
  R_ignore_SIGPIPE = 0;
  return result;
}
