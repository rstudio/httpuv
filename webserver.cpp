#include "webserver.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <iostream>
#include <sstream>

// TODO: Streaming response body (with chunked transfer encoding)
// TODO: Fast/easy use of files as response body

static uv_tcp_t server;
static http_parser_settings settings;
static uv_buf_t resbuf;

#define RESPONSE \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/plain\r\n" \
	"Content-Length: 12\r\n" \
	"\r\n" \
	"hello world\n"

void on_response_written(uv_write_t* handle, int status) {
    if (status != 0)
        std::cerr << "Error writing response: " << status << std::endl;
    delete (HttpResponse*)handle->data;
    free(handle);
}

uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size) {
    void* result = malloc(suggested_size);
    return uv_buf_init((char*)result, suggested_size);
}

void init_settings() {
    settings.on_message_begin = HttpRequest_on_message_begin;
    settings.on_url = HttpRequest_on_url;
    settings.on_status_complete = HttpRequest_on_status_complete;
    settings.on_header_field = HttpRequest_on_header_field;
    settings.on_header_value = HttpRequest_on_header_value;
    settings.on_headers_complete = HttpRequest_on_headers_complete;
    settings.on_body = HttpRequest_on_body;
    settings.on_message_complete = HttpRequest_on_message_complete;
}

void HttpRequest::trace(const std::string& msg) {
    //std::cerr << msg << std::endl;
}

uv_tcp_t* HttpRequest::handle() {
    return &_handle;
}

int HttpRequest::_on_message_begin(http_parser* pParser) {
    trace("on_message_begin");
    _headers.clear();
    return 0;
}

int HttpRequest::_on_url(http_parser* pParser, const char* pAt, size_t length) {
    trace("on_url");
    return 0;
}

int HttpRequest::_on_status_complete(http_parser* pParser) {
    trace("on_status_complete");
    return 0;
}
int HttpRequest::_on_header_field(http_parser* pParser, const char* pAt, size_t length) {
    trace("on_header_field");
    _lastHeaderField = std::string(pAt, length);
    return 0;
}

int HttpRequest::_on_header_value(http_parser* pParser, const char* pAt, size_t length) {
    trace("on_header_value");
    _headers[_lastHeaderField] = std::string(pAt, length);
    return 0;
}

int HttpRequest::_on_headers_complete(http_parser* pParser) {
    trace("on_headers_complete");
    for (std::map<std::string, std::string>::iterator iter = _headers.begin();
         iter != _headers.end();
         iter++) {
        std::cout << iter->first << std::string(" = ") << iter->second << std::endl;
    }
    return 0;
}

int HttpRequest::_on_body(http_parser* pParser, const char* pAt, size_t length) {
    trace("on_body");
    _bytesRead += length;
    return 0;
}

int HttpRequest::_on_message_complete(http_parser* pParser) {
    trace("on_message_complete");
    std::cout << _bytesRead << " bytes read" << std::endl;
    _headers.clear();

    HttpResponse* pResp = new HttpResponse(this, 200, "OK", resbuf.base, resbuf.len);
    pResp->addHeader("Content-Type", "text/plain");
    uv_write_t* pWriteReq = new uv_write_t();
    pResp->writeResponse(pWriteReq, &on_response_written);

    beginWrite((uv_stream_t*)&_handle, resbuf.base, resbuf.len, this);

    return 0;
}

void HttpRequest::fatal_error(const char* method, const char* message) {
    fprintf(stderr, "ERROR: [%s] %s\n", method, message);
}

void HttpRequest::_on_closed(uv_handle_t* handle) {
    printf("Closed\n");
    delete this;
}

void HttpRequest::close() {
    uv_close((uv_handle_t*)&_handle, HttpRequest_on_closed);
}

void HttpRequest::execute(uv_stream_t* stream) {
}

void HttpRequest::_on_request_read(uv_stream_t*, ssize_t nread, uv_buf_t buf) {
    if (nread > 0) {
        std::cerr << nread << " bytes read\n";
        int parsed = http_parser_execute(&_parser, &settings, buf.base, nread);
        if (_parser.upgrade) {
            // TODO: Handle HTTP upgrade
        } else if (parsed < nread) {
            fatal_error("on_request_read", "parse error");
            close();
        }
    } else if (nread < 0) {
        uv_err_t err = uv_last_error(_pLoop);
        if (err.code == UV_EOF || err.code == UV_ECONNRESET) {
        } else {
            fatal_error("on_request_read", uv_strerror(err));
        }
        close();
    } else {
        // It's normal for nread == 0, it's when uv requests a buffer then
        // decides it doesn't need it after all
    }

    free(buf.base);
}

void HttpRequest::handleRequest() {
    int r = uv_read_start((uv_stream_t*)&_handle, &on_alloc, &HttpRequest_on_request_read);
    if (r) {
        uv_err_t err = uv_last_error(_pLoop);
        fatal_error("read_start", uv_strerror(err));
        return;
    }
}

void HttpRequest::onWrite(int status) {
    std::cerr << "Response written: " << status << std::endl;
    if (status != 0) {
        uv_err_t err = uv_last_error(_pLoop);
        fatal_error("on_write", uv_strerror(err));
    }
}


void HttpResponse::addHeader(const std::string& name, const std::string& value) {
    _headers.push_back(std::pair<std::string, std::string>(name, value));
}

void HttpResponse::writeResponse(uv_write_t *req, uv_write_cb callback) {
    // TODO: Optimize
    std::ostringstream response(std::ios_base::binary);
    response << "HTTP/1.1 " << _statusCode << " " << _status << "\r\n";
    for (std::vector<std::pair<std::string, std::string> >::iterator it = _headers.begin();
         it != _headers.end();
         it++) {
        response << it->first << ": " << it->second << "\r\n";
    }
    response << "Content-Length: " << _body.len << "\r\n";
    response << "\r\n";
    std::string responseStr = response.str();
    _responseHeader.assign(responseStr.begin(), responseStr.end());

    uv_buf_t headerBuf = uv_buf_init(&_responseHeader[0], _responseHeader.size());
    uv_buf_t buffers[] = {headerBuf, this->_body};

    int r = uv_write(req, (uv_stream_t*)this->_pRequest->handle(), buffers, 2, callback);
    if (r) {
        _pRequest->fatal_error("uv_write",
                               uv_strerror(uv_last_error(_pRequest->handle()->loop)));
    }
}


#define IMPLEMENT_CALLBACK_1(type, function_name, return_type, type_1) \
    return_type type##_##function_name(type_1 arg1) { \
        return ((type*)(arg1->data))->_##function_name(arg1); \
    }
#define IMPLEMENT_CALLBACK_2(type, function_name, return_type, type_1, type_2) \
    return_type type##_##function_name(type_1 arg1, type_2 arg2) { \
        return ((type*)(arg1->data))->_##function_name(arg1, arg2); \
    }
#define IMPLEMENT_CALLBACK_3(type, function_name, return_type, type_1, type_2, type_3) \
    return_type type##_##function_name(type_1 arg1, type_2 arg2, type_3 arg3) { \
        return ((type*)(arg1->data))->_##function_name(arg1, arg2, arg3); \
    }

IMPLEMENT_CALLBACK_1(HttpRequest, on_message_begin, int, http_parser*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_url, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(HttpRequest, on_status_complete, int, http_parser*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_header_field, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_3(HttpRequest, on_header_value, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(HttpRequest, on_headers_complete, int, http_parser*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_body, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(HttpRequest, on_message_complete, int, http_parser*)
IMPLEMENT_CALLBACK_1(HttpRequest, on_closed, void, uv_handle_t*)
IMPLEMENT_CALLBACK_3(HttpRequest, on_request_read, void, uv_stream_t*, ssize_t, uv_buf_t)

void on_connect(uv_stream_t* handle, int status) {
	assert(handle == (uv_stream_t*)&server);
	printf("on_connect\n");

    if (status == -1) {
        uv_err_t err = uv_last_error(handle->loop);
        fprintf(stderr, "connection error: %s\n", uv_strerror(err));
        return;
    }

    HttpRequest* req = new HttpRequest(handle->loop);

    int r = uv_accept(handle, (uv_stream_t*)req->handle());
	if (r) {
        uv_err_t err = uv_last_error(handle->loop);
		fprintf(stderr, "accept: %s\n", uv_strerror(err));
        delete req;
		return;
	}

    req->handleRequest();

}

int main() {
    init_settings();

    resbuf = uv_buf_init(const_cast<char*>("hello"), 5);

	uv_tcp_init(uv_default_loop(), &server);
	struct sockaddr_in address = uv_ip4_addr("0.0.0.0", 8000);
	int r = uv_tcp_bind(&server, address);
	if (r) {
		uv_err_t err = uv_last_error(uv_default_loop());
		fprintf(stderr, "bind: %s\n", uv_strerror(err));
		return -1;
	}
	r = uv_listen((uv_stream_t*)&server, 128, &on_connect);
	if (r) {
		uv_err_t err = uv_last_error(uv_default_loop());
		fprintf(stderr, "listen: %s\n", uv_strerror(err));
		return -1;
	}

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);


	return 0;
}

typedef struct {
    uv_write_t handle;
    uv_buf_t data;
    WriteCallback* callback;
} write_req_t;

static void endWrite(uv_write_t* write, int status) {
    write_req_t* req = reinterpret_cast<write_req_t*>(write);
    req->callback->onWrite(status);
    free(req);
}

void beginWrite(uv_stream_t *stream, const char *pData, size_t length,
                WriteCallback* callback) {
    write_req_t* req = (write_req_t*)malloc(sizeof(write_req_t));
    memset(req, 0, sizeof(write_req_t));

    req->data = uv_buf_init(const_cast<char*>(pData), length);
    req->callback = callback;
    uv_write(&req->handle, stream, &req->data, 1, &endWrite);
}
