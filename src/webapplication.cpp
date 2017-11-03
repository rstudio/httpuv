#include <boost/bind.hpp>
#include "filedatasource.h"
#include "webapplication.h"
#include "httprequest.h"
#include "http.h"
#include "debug.h"


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


void requestToEnv(HttpRequest* pRequest, Rcpp::Environment* pEnv) {
  ASSERT_MAIN_THREAD()
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

HttpResponse* listToResponse(HttpRequest* pRequest, const Rcpp::List& response) {
  ASSERT_MAIN_THREAD()
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

void invokeResponseFun(boost::function<void(HttpResponse*)> fun, HttpRequest* pRequest, Rcpp::List response) {
  ASSERT_MAIN_THREAD()
  HttpResponse* pResponse = listToResponse(pRequest, response);
  fun(pResponse);
};


void RWebApplication::onHeaders(HttpRequest* pRequest, boost::function<void(HttpResponse*)> callback) {
  ASSERT_MAIN_THREAD()
  if (_onHeaders.isNULL()) {
    callback(NULL);
  }

  requestToEnv(pRequest, &pRequest->env());

  // Call the R onHeaders function
  Rcpp::List response(_onHeaders(pRequest->env()));

  callback(listToResponse(pRequest, response));
}

void RWebApplication::onBodyData(HttpRequest* pRequest,
                        const char* pData, size_t length) {
  ASSERT_MAIN_THREAD()
  Rcpp::RawVector rawVector(length);
  std::copy(pData, pData + length, rawVector.begin());
  _onBodyData(pRequest->env(), rawVector);
}

void RWebApplication::getResponse(HttpRequest* pRequest, boost::function<void(HttpResponse*)> callback) {
  ASSERT_MAIN_THREAD()
  using namespace Rcpp;

  boost::function<void(List)> * callback_wrapper = new boost::function<void(List)>();

  *callback_wrapper = boost::bind(
    &invokeResponseFun,
    callback,
    pRequest,
    _1
  );

  XPtr< boost::function<void(List)> > callback_xptr(callback_wrapper);

  _onRequest(pRequest->env(), callback_xptr);
}

void RWebApplication::onWSOpen(HttpRequest* pRequest) {
  ASSERT_MAIN_THREAD()
  requestToEnv(pRequest, &pRequest->env());
  _onWSOpen(externalize<WebSocketConnection>(pRequest->websocket()), pRequest->env());
}

void RWebApplication::onWSMessage(WebSocketConnection* pConn, bool binary, const char* data, size_t len) {
  ASSERT_MAIN_THREAD()
  if (binary)
    _onWSMessage(externalize<WebSocketConnection>(pConn), binary, std::vector<uint8_t>(data, data + len));
  else
    _onWSMessage(externalize<WebSocketConnection>(pConn), binary, std::string(data, len));
}

void RWebApplication::onWSClose(WebSocketConnection* pConn) {
  ASSERT_MAIN_THREAD()
  _onWSClose(externalize<WebSocketConnection>(pConn));
}
