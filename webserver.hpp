#include <map>
#include <string>
#include <iostream>

#include <uv.h>
#include <http_parser.h>

class WriteCallback {
public:
    virtual void onWrite(int status) = 0;
};

class HttpRequest : public WriteCallback {

private:
    uv_loop_t* _pLoop;
    uv_tcp_t _handle;
    http_parser _parser;
    uv_write_t _writeReq;
    std::map<std::string, std::string> _headers;
    std::string _lastHeaderField;
    unsigned long _bytesRead;

    void trace(const std::string& msg);

public:
    HttpRequest(uv_loop_t* pLoop) : _bytesRead(0) {
        _pLoop = pLoop;

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
    void execute(uv_stream_t* stream);
    void _on_request_read(uv_stream_t*, ssize_t nread, uv_buf_t buf);
    void _on_response_write(int status);

};

class HttpResponse {
private:
    int _status_code;
    std::string _status;
    std::map<std::string, std::string> _headers;
};

#define DECLARE_CALLBACK_1(function_name, return_type, type_1) \
    return_type function_name(type_1 arg1);
#define DECLARE_CALLBACK_3(function_name, return_type, type_1, type_2, type_3) \
    return_type function_name(type_1 arg1, type_2 arg2, type_3 arg3);
#define DECLARE_CALLBACK_2(function_name, return_type, type_1, type_2) \
    return_type function_name(type_1 arg1, type_2 arg2);

DECLARE_CALLBACK_1(on_message_begin, int, http_parser*)
DECLARE_CALLBACK_3(on_url, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(on_status_complete, int, http_parser*)
DECLARE_CALLBACK_3(on_header_field, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_3(on_header_value, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(on_headers_complete, int, http_parser*)
DECLARE_CALLBACK_3(on_body, int, http_parser*, const char*, size_t)
DECLARE_CALLBACK_1(on_message_complete, int, http_parser*)
DECLARE_CALLBACK_1(on_closed, void, uv_handle_t*)
DECLARE_CALLBACK_3(on_request_read, void, uv_stream_t*, ssize_t, uv_buf_t)
DECLARE_CALLBACK_2(on_response_write, void, uv_write_t*, int)

void beginWrite(uv_stream_t* stream, const char* pData, size_t length,
                WriteCallback* pCallback);
