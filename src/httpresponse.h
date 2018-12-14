#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "uvutil.h"
#include "utils.h"
#include "constants.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

class HttpRequest;

class HttpResponse : public boost::enable_shared_from_this<HttpResponse>  {

  boost::shared_ptr<HttpRequest> _pRequest;
  int _statusCode;
  std::string _status;
  ResponseHeaders _headers;
  std::vector<char> _responseHeader;
  boost::shared_ptr<DataSource> _pBody;
  bool _closeAfterWritten;

public:
  HttpResponse(boost::shared_ptr<HttpRequest> pRequest,
               int statusCode,
               const std::string& status,
               boost::shared_ptr<DataSource> pBody)
    : _pRequest(pRequest),
      _statusCode(statusCode),
      _status(status),
      _pBody(pBody),
      _closeAfterWritten(false)
  {
    _headers.push_back(std::make_pair("Date", http_date_string(time(NULL))));
  }

  ~HttpResponse();
  ResponseHeaders& headers();

  void addHeader(const std::string& name, const std::string& value);
  void setHeader(const std::string& name, const std::string& value);
  void writeResponse();
  void onResponseWritten(int status);
  void closeAfterWritten();
};

#endif
