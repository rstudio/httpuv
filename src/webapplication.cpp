#include <functional>
#include <memory>
#include "httpuv.h"
#include "filedatasource.h"
#include "webapplication.h"
#include "httprequest.h"
#include "http.h"
#include "thread.h"
#include "utils.h"
#include "mime.h"
#include "staticpath.h"
#include "fs.h"
#include <Rinternals.h>

// ============================================================================
// Utility functions
// ============================================================================

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

// An analog to errorResponse, but this returns an shared_ptr<HttpResponse>
// instead of an Rcpp::List, doesn't involve any R objects, and can be run on
// the background thread.
std::shared_ptr<HttpResponse> error_response(std::shared_ptr<HttpRequest> pRequest, int code) {
  std::string description = getStatusDescription(code);
  std::string content = toString(code) + " " + description + "\n";

  std::vector<uint8_t> responseData(content.begin(), content.end());

  // Freed in on_response_written
  std::shared_ptr<DataSource> pDataSource = std::make_shared<InMemoryDataSource>(responseData);

  return std::shared_ptr<HttpResponse>(
    new HttpResponse(pRequest, code, description, pDataSource),
    auto_deleter_background<HttpResponse>
  );
}

// Given a URL path like "/foo?abc=123", removes the '?' and everything after.
std::pair<std::string, std::string> splitQueryString(const std::string& url) {
  size_t qsIndex = url.find('?');
  std::string path, queryString;
  if (qsIndex == std::string::npos)
    path = url;
  else {
    path = url.substr(0, qsIndex);
    queryString = url.substr(qsIndex);
  }

  return std::pair<std::string, std::string>(path, queryString);
}


void requestToEnv(std::shared_ptr<HttpRequest> pRequest, Rcpp::Environment* pEnv) {
  ASSERT_MAIN_THREAD()
  using namespace Rcpp;

  Environment& env = *pEnv;

  std::pair<std::string, std::string> url_query = splitQueryString(pRequest->url());
  std::string& path        = url_query.first;
  std::string& queryString = url_query.second;

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


std::shared_ptr<HttpResponse> listToResponse(
  std::shared_ptr<HttpRequest> pRequest,
  const Rcpp::List& response)
{
  ASSERT_MAIN_THREAD()
  using namespace Rcpp;

  if (response.isNULL() || response.size() == 0) {
    return std::shared_ptr<HttpResponse>();
  }

  CharacterVector names = response.names();

  int status = Rcpp::as<int>(response["status"]);
  std::string statusDesc = getStatusDescription(status);

  List responseHeaders = response["headers"];

  // Self-frees when response is written
  std::shared_ptr<DataSource> pDataSource;

  // HTTP 1xx, 204 and 304 responses (as well as responses to HEAD requests)
  // cannot have a body, so permit the body element to be missing or NULL, in
  // which case pDataSource will be nullptr.
  //
  // See https://tools.ietf.org/html/rfc7231#section-6.3.5 and
  //     https://tools.ietf.org/html/rfc7232#section-4.1
  bool hasBody = response.containsElementNamed("body") && !Rf_isNull(response["body"]);

  // The response can either contain:
  // - bodyFile: String value that names the file that should be streamed
  // - body: Character vector (which is charToRaw-ed) or raw vector, or NULL
  if (std::find(names.begin(), names.end(), "bodyFile") != names.end()) {
    std::shared_ptr<FileDataSource> pFDS = std::make_shared<FileDataSource>();
    FileDataSourceResult ret = pFDS->initialize(
      Rcpp::as<std::string>(response["bodyFile"]),
      Rcpp::as<bool>(response["bodyFileOwned"])
    );
    if (ret != FDS_OK) {
      REprintf(pFDS->lastErrorMessage().c_str());
      return error_response(pRequest, 500);
    }
    pDataSource = pFDS;
  }
  else if (hasBody && Rf_isString(response["body"])) {
    RawVector responseBytes = Function("charToRaw")(response["body"]);
    pDataSource = std::make_shared<InMemoryDataSource>(responseBytes);
  }
  else if (hasBody) {
    RawVector responseBytes = response["body"];
    pDataSource = std::make_shared<InMemoryDataSource>(responseBytes);
  }

  std::shared_ptr<HttpResponse> pResp(
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

void invokeResponseFun(std::function<void(std::shared_ptr<HttpResponse>)> fun,
                       std::shared_ptr<HttpRequest> pRequest,
                       Rcpp::List response)
{
  ASSERT_MAIN_THREAD()
  // new HttpResponse object. The callback will invoke
  // HttpResponse->writeResponse().
  std::shared_ptr<HttpResponse> pResponse = listToResponse(pRequest, response);
  fun(pResponse);
}


// ============================================================================
// Methods
// ============================================================================

RWebApplication::RWebApplication(
    Rcpp::Function onHeaders,
    Rcpp::Function onBodyData,
    Rcpp::Function onRequest,
    Rcpp::Function onWSOpen,
    Rcpp::Function onWSMessage,
    Rcpp::Function onWSClose,
    Rcpp::List     staticPaths,
    Rcpp::List     staticPathOptions) :
    _onHeaders(onHeaders), _onBodyData(onBodyData), _onRequest(onRequest),
    _onWSOpen(onWSOpen), _onWSMessage(onWSMessage), _onWSClose(onWSClose)
{
  ASSERT_MAIN_THREAD()

  _staticPathManager = StaticPathManager(staticPaths, staticPathOptions);
}


void RWebApplication::onHeaders(std::shared_ptr<HttpRequest> pRequest,
                                std::function<void(std::shared_ptr<HttpResponse>)> callback)
{
  ASSERT_MAIN_THREAD()
  if (_onHeaders.isNULL()) {
    std::shared_ptr<HttpResponse> null_ptr;
    callback(null_ptr);
  }

  requestToEnv(pRequest, &pRequest->env());

  // Call the R onHeaders function. If an exception occurs during processing,
  // catch it and then send a generic error response.
  Rcpp::List response;
  try {
    response = _onHeaders(pRequest->env());
  } catch (Rcpp::internal::InterruptedException &e) {
    debug_log("Interrupt occurred in _onHeaders", LOG_INFO);
    response = errorResponse();
  } catch (...) {
    debug_log("Exception occurred in _onHeaders", LOG_INFO);
    response = errorResponse();
  }

  // new HttpResponse object. The callback will invoke
  // HttpResponse->writeResponse(), which adds a callback to destroy(), which
  // deletes the object.
  std::shared_ptr<HttpResponse> pResponse = listToResponse(pRequest, response);
  callback(pResponse);
}

void RWebApplication::onBodyData(std::shared_ptr<HttpRequest> pRequest,
      std::shared_ptr<std::vector<char> > data,
      std::function<void(std::shared_ptr<HttpResponse>)> errorCallback)
{
  ASSERT_MAIN_THREAD()
  debug_log("RWebApplication::onBodyData", LOG_DEBUG);

  // We're in an error state, but the background thread has already scheduled
  // more data to be processed here. Don't process more data.
  if (pRequest->isResponseScheduled())
    return;

  Rcpp::RawVector rawVector(data->size());
  std::copy(data->begin(), data->end(), rawVector.begin());
  try {
    _onBodyData(pRequest->env(), rawVector);
  } catch (...) {
    debug_log("Exception occurred in _onBodyData", LOG_INFO);
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

void RWebApplication::getResponse(std::shared_ptr<HttpRequest> pRequest,
                                  std::function<void(std::shared_ptr<HttpResponse>)> callback) {
  ASSERT_MAIN_THREAD()
  debug_log("RWebApplication::getResponse", LOG_DEBUG);
  using namespace Rcpp;

  // Pass callback to R:
  // invokeResponseFun(callback, pRequest, _1)
  std::function<void(List)>* callback_wrapper = new std::function<void(List)>(
    std::bind(invokeResponseFun, callback, pRequest, std::placeholders::_1)
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
      debug_log("Interrupt occurred in _onRequest", LOG_INFO);
      invokeCppCallback(errorResponse(), callback_xptr);
    } catch (...) {
      debug_log("Exception occurred in _onRequest", LOG_INFO);
      invokeCppCallback(errorResponse(), callback_xptr);
    }
  }

  UNPROTECT(1);
}

void RWebApplication::onWSOpen(std::shared_ptr<HttpRequest> pRequest,
                               std::function<void(void)> error_callback) {
  ASSERT_MAIN_THREAD()
  std::shared_ptr<WebSocketConnection> pConn = pRequest->websocket();
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

void RWebApplication::onWSMessage(std::shared_ptr<WebSocketConnection> pConn,
                                  bool binary,
                                  std::shared_ptr<std::vector<char> > data,
                                  std::function<void(void)> error_callback)
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

void RWebApplication::onWSClose(std::shared_ptr<WebSocketConnection> pConn) {
  ASSERT_MAIN_THREAD()
  _onWSClose(externalize_shared_ptr(pConn));
}


// ============================================================================
// Static file serving
// ============================================================================
//
// Unlike most of the methods for an RWebApplication, these ones are called on
// the background thread.

std::shared_ptr<HttpResponse> RWebApplication::staticFileResponse(
  std::shared_ptr<HttpRequest> pRequest
) {
  ASSERT_BACKGROUND_THREAD()

  // If there's any Upgrade header, don't try to serve a static file. Just
  // fall through, even if the path is one that is in the StaticPathManager.
  if (pRequest->hasHeader("Upgrade")) {
    return std::shared_ptr<HttpResponse>();
  }

  // Strip off query string
  std::pair<std::string, std::string> url_query = splitQueryString(pRequest->url());
  std::string url_path = doDecodeURI(url_query.first, true);

  std::experimental::optional<std::pair<StaticPath, std::string>> sp_pair =
    _staticPathManager.matchStaticPath(url_path);

  if (!sp_pair) {
    // This was not a static path. Fall through to the R code to handle this
    // path.
    return std::shared_ptr<HttpResponse>();
  }

  // If we get here, we've matched a static path.

  const StaticPath&  sp      = sp_pair->first;
  // Note that the subpath may include leading dirs, as in "foo/bar/abc.txt".
  const std::string& subpath = sp_pair->second;

  // This is an excluded path
  if (*sp.options.exclude) {
    return std::shared_ptr<HttpResponse>();
  }

  // Validate headers (if validation pattern was provided).
  if (!sp.options.validateRequestHeaders(pRequest->headers())) {
    return error_response(pRequest, 403);
  }

  // Check that method is GET or HEAD; error otherwise.
  std::string method = pRequest->method();
  if (method != "GET" && method != "HEAD") {
    return error_response(pRequest, 400);
  }

  // Make sure that there's no message body.
  if ((pRequest->hasHeader("Content-Length") && pRequest->getHeader("Content-Length") != "0")
        || pRequest->hasHeader("Transfer-Encoding")) {
    return error_response(pRequest, 400);
  }

  // Disallow ".." in paths. (Browsers collapse them anyway, so no normal
  // requests should contain them.) The ones we care about will always be
  // between two slashes, as in "/foo/../bar", except in the case where it's
  // at the end of the URL, as in "/foo/..". Paths like "/foo../" or "/..foo/"
  // are OK.
  if (url_path.find("/../") != std::string::npos ||
      (url_path.length() >= 3 && url_path.substr(url_path.length()-3, 3) == "/..")
  ) {
    if (*sp.options.fallthrough) {
      return std::shared_ptr<HttpResponse>();
    } else {
      return error_response(pRequest, 400);
    }
  }

  // Path to local file on disk
  std::string local_path = sp.path;
  if (subpath != "") {
    local_path += "/" + subpath;
  }

  if (is_directory(local_path)) {
    if (*sp.options.indexhtml) {
      local_path = local_path + "/" + "index.html";
    }
  }

  std::shared_ptr<FileDataSource> pDataSource = std::make_shared<FileDataSource>();
  FileDataSourceResult ret = pDataSource->initialize(local_path, false);

  if (ret != FDS_OK) {
    if (ret == FDS_NOT_EXIST || ret == FDS_ISDIR) {
      if (*sp.options.fallthrough) {
        return std::shared_ptr<HttpResponse>();
      } else {
        return error_response(pRequest, 404);
      }
    } else {
      return error_response(pRequest, 500);
    }
  }

  // Use local_path instead of subpath, because if the subpath is "/foo/" and
  // *(sp.options.indexhtml) is true, then the local_path will be
  // "/foo/index.html". We need to use the latter to determine mime type.
  std::string content_type = find_mime_type(find_extension(basename(local_path)));
  if (content_type == "") {
    content_type = "application/octet-stream";
  } else if (content_type == "text/html") {
    // Add the encoding if specified by the options.
    if (*sp.options.html_charset != "") {
      content_type = "text/html; charset=" + *sp.options.html_charset;
    }
  }

  // Check if the client has an up-to-date copy of the file in cache. To do
  // this, compare the If-Modified-Since header to the file's mtime.
  bool client_cache_is_valid = false;
  if (pRequest->hasHeader("If-Modified-Since")) {
    time_t file_mtime = pDataSource->getMtime();
    time_t if_mod_since = parse_http_date_string(pRequest->getHeader("If-Modified-Since"));

    if (file_mtime != 0 && if_mod_since != 0 && file_mtime <= if_mod_since) {
      client_cache_is_valid = true;
    }
  }

  // ==================================
  // Create the HTTP response
  // ==================================

  // Default status code at this point is 200.
  int status_code = 200;

  // This is the pointer that will be passed to the new HttpResponse. We'll
  // start by setting it to point to the same thing as pDataSource, but it can
  // be unset based on various conditions, which means that no body data will
  // be sent.
  std::shared_ptr<FileDataSource> pDataSource2 = pDataSource;

  if (method == "HEAD") {
    pDataSource2.reset();
  }

  if (client_cache_is_valid) {
    pDataSource2.reset();
    status_code = 304;
  }

  std::shared_ptr<HttpResponse> pResponse = std::shared_ptr<HttpResponse>(
    new HttpResponse(pRequest, status_code, getStatusDescription(status_code), pDataSource2),
    auto_deleter_background<HttpResponse>
  );

  ResponseHeaders& respHeaders = pResponse->headers();

  // Add extra user-specified headers.
  const ResponseHeaders& extraRespHeaders = *sp.options.headers;
  if (extraRespHeaders.size() != 0) {
    ResponseHeaders::const_iterator it;
    for (it = extraRespHeaders.begin(); it != extraRespHeaders.end(); it++) {
      if (status_code == 304) {
        // For a 304 response, only a few headers should be added. See
        // https://tools.ietf.org/html/rfc7232#section-4.1
        // (Date is automatically added in the HttpResponse.)
        if (it->first == "Cache-Control" || it->first == "Content-Location" ||
            it->first == "ETag" || it->first == "Expires" || it->first == "Vary")
        {
          respHeaders.push_back(*it);
        }

      } else {
        // For a normal 200 response, add all headers.
        respHeaders.push_back(*it);
      }
    }
  }

  if (status_code != 304) {
    // Set the Content-Length here so that both GET and HEAD requests will get
    // it. If we didn't set it here, the response for the GET would
    // automatically set the Content-Length (by using the FileDataSource), but
    // the response for the HEAD would not.
    respHeaders.push_back(std::make_pair("Content-Length", toString(pDataSource->size())));
    respHeaders.push_back(std::make_pair("Content-Type", content_type));
    respHeaders.push_back(std::make_pair("Last-Modified", http_date_string(pDataSource->getMtime())));
  }

  return pResponse;
}

StaticPathManager& RWebApplication::getStaticPathManager() {
  return _staticPathManager;
}
