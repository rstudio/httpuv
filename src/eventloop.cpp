#include "eventloop.hpp"
#include <stdio.h>
#include <map>
#include <string>
#include <uv.h>
#include "http.hpp"

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

    RawVector responseBytes((_onRequest)(env));
    std::vector<char> resp(responseBytes.size());
    resp.assign(responseBytes.begin(), responseBytes.end());
    return new HttpResponse(pRequest, 200, "OK", resp);
  }

  void onWSOpen(WebSocketConnection* pConn) {
    _onWSOpen((intptr_t)pConn);
  }

  void onWSMessage(WebSocketConnection* pConn, bool binary, const char* data, size_t len) {
    if (binary)
      _onWSMessage((intptr_t)pConn, binary, std::vector<char>(data, data + len));
    else
      _onWSMessage((intptr_t)pConn, binary, std::string(data, len));
  }
  
  void onWSClose(WebSocketConnection* pConn) {
    _onWSClose((intptr_t)pConn);
  }

};

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
  return runNonBlocking(uv_default_loop());
}
