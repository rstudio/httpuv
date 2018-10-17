#include <boost/bind.hpp>
#include "httpuv.h"
#include "filedatasource.h"
#include "webapplication.h"
#include "httprequest.h"
#include "http.h"
#include "thread.h"
#include "utils.h"
#include "mime.h"
#include <Rinternals.h>

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

// A generic HTTP response to send when an error (uncaught in the R code)
// happens during processing a request.
Rcpp::List errorResponse() {
  ASSERT_MAIN_THREAD()
  using namespace Rcpp;
  return List::create(
    _["status"] = 500L,
    _["headers"] = List::create(
      _["Content-Type"] = "text/plain; charset=UTF-8"
    ),
    _["body"] = "An exception occurred."
  );
}

void requestToEnv(boost::shared_ptr<HttpRequest> pRequest, Rcpp::Environment* pEnv) {
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

  // When making assignments into the Environment, the value must be wrapped
  // in a Rcpp object -- letting Rcpp automatically do the wrapping can result
  // in an object being GC'd too early.
  // https://github.com/RcppCore/Rcpp/issues/780
  env["REQUEST_METHOD"] = CharacterVector(pRequest->method());
  env["SCRIPT_NAME"] = CharacterVector(std::string(""));
  env["PATH_INFO"] = CharacterVector(path);
  env["QUERY_STRING"] = CharacterVector(queryString);

  env["rook.version"] = CharacterVector("1.1-0");
  env["rook.url_scheme"] = CharacterVector("http");

  Address addr = pRequest->serverAddress();
  env["SERVER_NAME"] = CharacterVector(addr.host);
  std::ostringstream portstr;
  portstr << addr.port;
  env["SERVER_PORT"] = CharacterVector(portstr.str());

  Address raddr = pRequest->clientAddress();
  env["REMOTE_ADDR"] = CharacterVector(raddr.host);
  std::ostringstream rportstr;
  rportstr << raddr.port;
  env["REMOTE_PORT"] = CharacterVector(rportstr.str());

  const RequestHeaders& headers = pRequest->headers();
  Rcpp::CharacterVector raw_headers(headers.size());
  Rcpp::CharacterVector raw_header_names(headers.size());

  for (RequestHeaders::const_iterator it = headers.begin();
    it != headers.end();
    it++) {
    int idx = std::distance(headers.begin(), it);
    env["HTTP_" + normalizeHeaderName(it->first)] = CharacterVector(it->second);
    raw_header_names[idx] = to_lower(it->first);
    raw_headers[idx] = it->second;
  }
  raw_headers.attr("names") = raw_header_names;

  env["HEADERS"] = raw_headers;

}


boost::shared_ptr<HttpResponse> listToResponse(
  boost::shared_ptr<HttpRequest> pRequest,
  const Rcpp::List& response)
{
  ASSERT_MAIN_THREAD()
  using namespace Rcpp;

  if (response.isNULL() || response.size() == 0) {
    boost::shared_ptr<HttpResponse> null_ptr;
    return null_ptr;
  }

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
    int ret = pFDS->initialize(Rcpp::as<std::string>(response["bodyFile"]),
                               Rcpp::as<bool>(response["bodyFileOwned"]));
    if (ret != 0) {
      REprintf(pFDS->lastErrorMessage().c_str());
    }
    pDataSource = pFDS;
  }
  else if (Rf_isString(response["body"])) {
    RawVector responseBytes = Function("charToRaw")(response["body"]);
    pDataSource = new InMemoryDataSource(responseBytes);
  }
  else {
    RawVector responseBytes = response["body"];
    pDataSource = new InMemoryDataSource(responseBytes);
  }

  boost::shared_ptr<HttpResponse> pResp(
    new HttpResponse(pRequest, status, statusDesc, pDataSource),
    auto_deleter_background<HttpResponse>
  );
  CharacterVector headerNames = responseHeaders.names();
  for (R_len_t i = 0; i < responseHeaders.size(); i++) {
    pResp->addHeader(
      std::string((char*)headerNames[i], headerNames[i].size()),
      Rcpp::as<std::string>(responseHeaders[i]));
  }

  return pResp;
}

void invokeResponseFun(boost::function<void(boost::shared_ptr<HttpResponse>)> fun,
                       boost::shared_ptr<HttpRequest> pRequest,
                       Rcpp::List response)
{
  ASSERT_MAIN_THREAD()
  // new HttpResponse object. The callback will invoke
  // HttpResponse->writeResponse().
  boost::shared_ptr<HttpResponse> pResponse = listToResponse(pRequest, response);
  fun(pResponse);
}

RWebApplication::RWebApplication(
    Rcpp::Function onHeaders,
    Rcpp::Function onBodyData,
    Rcpp::Function onRequest,
    Rcpp::Function onWSOpen,
    Rcpp::Function onWSMessage,
    Rcpp::Function onWSClose,
    Rcpp::Function getStaticPaths) :
    _onHeaders(onHeaders), _onBodyData(onBodyData), _onRequest(onRequest),
    _onWSOpen(onWSOpen), _onWSMessage(onWSMessage), _onWSClose(onWSClose),
    _getStaticPaths(getStaticPaths)
{
  ASSERT_MAIN_THREAD()

  _staticPaths = toStringMap(Rcpp::CharacterVector(_getStaticPaths()));
}


void RWebApplication::onHeaders(boost::shared_ptr<HttpRequest> pRequest,
                                boost::function<void(boost::shared_ptr<HttpResponse>)> callback)
{
  ASSERT_MAIN_THREAD()
  if (_onHeaders.isNULL()) {
    boost::shared_ptr<HttpResponse> null_ptr;
    callback(null_ptr);
  }

  requestToEnv(pRequest, &pRequest->env());

  // Call the R onHeaders function. If an exception occurs during processing,
  // catch it and then send a generic error response.
  Rcpp::List response;
  try {
    response = _onHeaders(pRequest->env());
  } catch (Rcpp::internal::InterruptedException &e) {
    trace("Interrupt occurred in _onHeaders");
    response = errorResponse();
  } catch (...) {
    trace("Exception occurred in _onHeaders");
    response = errorResponse();
  }

  // new HttpResponse object. The callback will invoke
  // HttpResponse->writeResponse(), which adds a callback to destroy(), which
  // deletes the object.
  boost::shared_ptr<HttpResponse> pResponse = listToResponse(pRequest, response);
  callback(pResponse);
}

void RWebApplication::onBodyData(boost::shared_ptr<HttpRequest> pRequest,
      boost::shared_ptr<std::vector<char> > data,
      boost::function<void(boost::shared_ptr<HttpResponse>)> errorCallback)
{
  ASSERT_MAIN_THREAD()
  trace("RWebApplication::onBodyData");

  // We're in an error state, but the background thread has already scheduled
  // more data to be processed here. Don't process more data.
  if (pRequest->isResponseScheduled())
    return;

  Rcpp::RawVector rawVector(data->size());
  std::copy(data->begin(), data->end(), rawVector.begin());
  try {
    _onBodyData(pRequest->env(), rawVector);
  } catch (...) {
    trace("Exception occurred in _onBodyData");
    // Send an error message to the client. It's very possible that getResponse() or more
    // calls to onBodyData() will have been scheduled on the main thread
    // before the errorCallback is called.
    //
    // Note that some (most?) clients won't correctly handle a response that's
    // sent early, before the request is completed.
    // https://stackoverflow.com/a/18370751/412655
    errorCallback(listToResponse(pRequest, errorResponse()));
  }
}

void RWebApplication::getResponse(boost::shared_ptr<HttpRequest> pRequest,
                                  boost::function<void(boost::shared_ptr<HttpResponse>)> callback) {
  ASSERT_MAIN_THREAD()
  trace("RWebApplication::getResponse");
  using namespace Rcpp;

  // Pass callback to R:
  // invokeResponseFun(callback, pRequest, _1)
  boost::function<void(List)>* callback_wrapper = new boost::function<void(List)>(
    boost::bind(invokeResponseFun, callback, pRequest, _1)
  );

  SEXP callback_xptr = PROTECT(R_MakeExternalPtr(callback_wrapper, R_NilValue, R_NilValue));

  // We previously encountered an error processing the body. Don't call into
  // the R call/_onRequest() function. We need to signal the HttpRequest
  // object to let it know that we had an error.
  if (pRequest->isResponseScheduled()) {
    invokeCppCallback(Rcpp::List(), callback_xptr);
  }
  else {

    // Call the R call() function, and pass it the callback xptr so it can
    // asynchronously pass data back to C++.
    try {
      _onRequest(pRequest->env(), callback_xptr);

      // On the R side, httpuv's call() function will catch errors that happen
      // in the user-defined call() function, but if an error happens outside of
      // that scope, or if another uncaught exception happens (like an interrupt
      // if Ctrl-C is pressed), then it will bubble up to here, where we'll catch
      // it and deal with it.

    } catch (Rcpp::internal::InterruptedException &e) {
      trace("Interrupt occurred in _onRequest");
      invokeCppCallback(errorResponse(), callback_xptr);
    } catch (...) {
      trace("Exception occurred in _onRequest");
      invokeCppCallback(errorResponse(), callback_xptr);
    }
  }

  UNPROTECT(1);
}

void RWebApplication::onWSOpen(boost::shared_ptr<HttpRequest> pRequest,
                               boost::function<void(void)> error_callback) {
  ASSERT_MAIN_THREAD()
  boost::shared_ptr<WebSocketConnection> pConn = pRequest->websocket();
  if (!pConn) {
    return;
  }

  requestToEnv(pRequest, &pRequest->env());
  try {
    _onWSOpen(
      externalize_shared_ptr(pConn),
      pRequest->env()
    );
  } catch(...) {
    error_callback();
  }
}

void RWebApplication::onWSMessage(boost::shared_ptr<WebSocketConnection> pConn,
                                  bool binary,
                                  boost::shared_ptr<std::vector<char> > data,
                                  boost::function<void(void)> error_callback)
{
  ASSERT_MAIN_THREAD()
  try {
    if (binary)
      _onWSMessage(
        externalize_shared_ptr(pConn),
        binary,
        std::vector<uint8_t>(data->begin(), data->end())
      );
    else
      _onWSMessage(
        externalize_shared_ptr(pConn),
        binary,
        std::string(data->begin(), data->end())
      );
  } catch(...) {
    error_callback();
  }
}

void RWebApplication::onWSClose(boost::shared_ptr<WebSocketConnection> pConn) {
  ASSERT_MAIN_THREAD()
  _onWSClose(externalize_shared_ptr(pConn));
}


// ============================================================================
// Static file serving
// ============================================================================
//
// Unlike most of the methods for an RWebApplication, these ones are called on
// the background thread.


// Returns a pair where the first element is the local directory, and the
// second element is the filename.
std::pair<std::string, std::string> RWebApplication::_matchStaticPath(const std::string& url_path) const {
  ASSERT_BACKGROUND_THREAD()

  std::string path = url_path;
  size_t last_split_idx = std::string::npos;

  // This loop splits the string on '/', starting with the last one, and then
  // searches for a match in _staticPaths of the first part. If found, it
  // returns a pair with the part before the slash, and the part after the
  // slash. If not found, it splits on the previous '/' and searches again,
  // and so on, until there are no more to split on.
  while (true) {
    // Split the string on '/'
    size_t found_idx = path.find_last_of('/', last_split_idx);

    if (found_idx <= 0) {
      return std::pair<std::string, std::string>("", "");
    }

    std::string pre_slash  = path.substr(0, found_idx);
    std::string post_slash = path.substr(found_idx + 1);
  
    std::map<std::string, std::string>::const_iterator it = _staticPaths.find(pre_slash);
    if (it != _staticPaths.end()) {
      // Pair with dirname, filename
      return std::pair<std::string, std::string>(it->second, post_slash);
    }

    last_split_idx = found_idx - 1 ;
  }
}


boost::shared_ptr<HttpResponse> error_response(boost::shared_ptr<HttpRequest> pRequest, int code) {
  std::string description = getStatusDescription(code);
  std::string content = toString(code) + " " + description + "\n";

  std::vector<uint8_t> responseData(content.begin(), content.end());

  // Freed in on_response_written
  DataSource* pDataSource = new InMemoryDataSource(responseData);

  return boost::shared_ptr<HttpResponse>(
    new HttpResponse(pRequest, code, description, pDataSource),
    auto_deleter_background<HttpResponse>
  );
}


boost::shared_ptr<HttpResponse> RWebApplication::staticFileResponse(
  boost::shared_ptr<HttpRequest> pRequest
) {
  ASSERT_BACKGROUND_THREAD()

  std::string url_path = doDecodeURI(pRequest->url(), true);
  std::pair<std::string, std::string> static_path = _matchStaticPath(url_path);
  std::string local_dirname = static_path.first;
  std::string filename      = static_path.second;

  if (local_dirname == "") {
    // This was not a static path.
    return nullptr;
  }

  // Check that method is GET or HEAD; error otherwise.
  std::string method = pRequest->method();
  if (method != "GET" && method != "HEAD") {
    return error_response(pRequest, 400);
  }

  // Make sure that there's no message body.
  if (pRequest->hasHeader("Content-Length") || pRequest->hasHeader("Transfer-Encoding")) {
    return error_response(pRequest, 400);
  }

  std::string local_path = local_dirname + "/" + filename;

  // Self-frees when response is written
  FileDataSource* pDataSource = new FileDataSource();
  // TODO: Figure out how to deal with `owned` parameter.
  int ret = pDataSource->initialize(local_path, false);

  if (ret != 0) {
    // Couldn't read the file
    delete pDataSource;
    return error_response(pRequest, 404);
  }

  int file_size = pDataSource->size();
  std::string mime_type = find_mime_type(find_extension(filename));

  if (method == "HEAD") {
    // For HEAD requests, we created the FileDataSource to get the size and
    // validate that the file exists, but don't actually send the file's data.
    delete pDataSource;
    pDataSource = NULL;
  }

  boost::shared_ptr<HttpResponse> pResponse(
    new HttpResponse(pRequest, 200, getStatusDescription(200), pDataSource),
    auto_deleter_background<HttpResponse>
  );

  ResponseHeaders& respHeaders = pResponse->headers();
  // Set the Content-Length here so that both GET and HEAD requests will get
  // it. If we didn't set it here, the response for the GET would
  // automatically set the Content-Length (by using the FileDataSource), but
  // the response for the HEAD would not.
  respHeaders.push_back(std::make_pair("Content-Length", toString(file_size)));
  if (mime_type != "") {
    respHeaders.push_back(std::make_pair("Content-Type", mime_type));
  }

  return pResponse;
}


std::map<std::string, std::string> RWebApplication::getStaticPaths() const {
  // TODO: always lock staticPaths
  return _staticPaths;
};

std::map<std::string, std::string> RWebApplication::addStaticPaths(
  const std::map<std::string, std::string>& paths
) {

  std::map<std::string, std::string>::const_iterator it;
  for (it = paths.begin(); it != paths.end(); it++) {
    _staticPaths[it->first] = it->second;
  }

  return _staticPaths;
};

std::map<std::string, std::string> RWebApplication::removeStaticPaths(
  const std::vector<std::string>& paths
) {

  std::vector<std::string>::const_iterator path_it = paths.begin();

  for (path_it = paths.begin(); path_it != paths.end(); path_it++) {
    std::map<std::string, std::string>::const_iterator static_paths_it = _staticPaths.find(*path_it);
    if (static_paths_it == _staticPaths.end()) {
      err_printf("Tried to remove static path, but it wasn't present: %s\n", path_it->c_str());
    } else {
      _staticPaths.erase(static_paths_it);
    }
  }

  return _staticPaths;
};
