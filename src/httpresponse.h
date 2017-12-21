#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "uvutil.h"
#include "constants.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

class HttpRequest;

class HttpResponse {

  boost::shared_ptr<HttpRequest> _pRequest;
  int _statusCode;
  std::string _status;
  ResponseHeaders _headers;
  std::vector<char> _responseHeader;
  DataSource* _pBody;
  bool _closeAfterWritten;

public:
  HttpResponse(boost::shared_ptr<HttpRequest> pRequest,
               int statusCode,
               const std::string& status,
               DataSource* pBody)
    : _pRequest(pRequest),
      _statusCode(statusCode),
      _status(status),
      _pBody(pBody),
      _closeAfterWritten(false)
  {
  }

  virtual ~HttpResponse() {
    delete _pBody;
  }

  ResponseHeaders& headers();

  void addHeader(const std::string& name, const std::string& value);
  void setHeader(const std::string& name, const std::string& value);
  void writeResponse();
  void onResponseWritten(int status);
  void closeAfterWritten();
  void destroy(bool forceClose = false);
};


#endif
