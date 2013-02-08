#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <map>
#include <string>
#include <iostream>
#include <vector>

#include <uv.h>
#include <http_parser.h>

class HttpRequest;
class HttpResponse;

class WriteCallback {
public:
    virtual ~WriteCallback() {}
    virtual void onWrite(int status) = 0;
};

class RequestHandler {
public:
    virtual ~RequestHandler() {}
    virtual HttpResponse* getResponse(HttpRequest* request) = 0;
};

class HttpRequest : public WriteCallback {

private:
    uv_loop_t* _pLoop;
    RequestHandler* _pRequestHandler;
    uv_tcp_t _handle;
    http_parser _parser;
    uv_write_t _writeReq;
    std::string _url;
    std::map<std::string, std::string> _headers;
    std::string _lastHeaderField;
    unsigned long _bytesRead;

    void trace(const std::string& msg);

public:
    HttpRequest(uv_loop_t* pLoop, RequestHandler* pRequestHandler)
        : _bytesRead(0) {
        _pLoop = pLoop;
        _pRequestHandler = pRequestHandler;

        uv_tcp_init(pLoop, &_handle);
        _handle.data = this;

        http_parser_init(&_parser, HTTP_REQUEST);
        _parser.data = this;

        _writeReq.data = this;
    }

    virtual ~HttpRequest() {
    }

    uv_tcp_t* handle();
    void handleRequest();

    std::string method() const;
    std::string url() const;
    std::map<std::string, std::string> headers() const;

public:
    // Callbacks
    virtual int _on_message_begin(http_parser* pParser);
    virtual int _on_url(http_parser* pParser, const char* pAt, size_t length);
    virtual int _on_status_complete(http_parser* pParser);
    virtual int _on_header_field(http_parser* pParser, const char* pAt, size_t length);
    virtual int _on_header_value(http_parser* pParser, const char* pAt, size_t length);
    virtual int _on_headers_complete(http_parser* pParser);
    virtual int _on_body(http_parser* pParser, const char* pAt, size_t length);
    virtual int _on_message_complete(http_parser* pParser);

    virtual void onWrite(int status);

    void write(const char* pData, size_t length, WriteCallback* pCallback);

    void fatal_error(const char* method, const char* message);
    void _on_closed(uv_handle_t* handle);
    void close();
    void _on_request_read(uv_stream_t*, ssize_t nread, uv_buf_t buf);
    void _on_response_write(int status);

};

struct HttpResponse {

    HttpRequest* _pRequest;
    int _statusCode;
    std::string _status;
    std::vector<std::pair<std::string, std::string> > _headers;
    std::vector<char> _responseHeader;
    char* _pBody;
    uv_buf_t _body;

public:
    HttpResponse(HttpRequest* pRequest, int statusCode,
                 const std::string& status, char* pBody, size_t length)
        : _pRequest(pRequest), _statusCode(statusCode), _status(status) {

        _body = uv_buf_init(pBody, length);
    }

    virtual ~HttpResponse() {
    }

    void addHeader(const std::string& name, const std::string& value);
    void writeResponse(uv_write_t* req, uv_write_cb callback);
};

#define DECLARE_CALLBACK_1(type, function_name, return_type, type_1) \
    return_type type##_##function_name(type_1 arg1);
#define DECLARE_CALLBACK_3(type, function_name, return_type, type_1, type_2, type_3) \
    return_type type##_##function_name(type_1 arg1, type_2 arg2, type_3 arg3);
#define DECLARE_CALLBACK_2(type, function_name, return_type, type_1, type_2) \
    return_type type##_##function_name(type_1 arg1, type_2 arg2);

DECLARE_CALLBACK_1(HttpRequest, on_message_begin, int, http_parser*)
DECLARE_CALLBACK_3(HttpRequest, on_url, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(HttpRequest, on_status_complete, int, http_parser*)
DECLARE_CALLBACK_3(HttpRequest, on_header_field, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_3(HttpRequest, on_header_value, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(HttpRequest, on_headers_complete, int, http_parser*)
DECLARE_CALLBACK_3(HttpRequest, on_body, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(HttpRequest, on_message_complete, int, http_parser*)
DECLARE_CALLBACK_1(HttpRequest, on_closed, void, uv_handle_t*)
DECLARE_CALLBACK_3(HttpRequest, on_request_read, void, uv_stream_t*, ssize_t, uv_buf_t)
DECLARE_CALLBACK_2(HttpRequest, on_response_write, void, uv_write_t*, int)

void beginWrite(uv_stream_t* stream, const char* pData, size_t length,
                WriteCallback* pCallback);

uv_tcp_t* createServer(uv_loop_t* loop, const std::string& host, int port,
    RequestHandler* pRequestHandler);
void freeServer(uv_tcp_t* pServer);
bool runNonBlocking(uv_loop_t* loop);

#endif // WEBSERVER_HPP
