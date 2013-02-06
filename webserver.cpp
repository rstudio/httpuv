#include "webserver.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <iostream>

static uv_tcp_t server;
static http_parser_settings settings;
static uv_buf_t resbuf;

#define RESPONSE \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/plain\r\n" \
	"Content-Length: 12\r\n" \
	"\r\n" \
	"hello world\n"

uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size) {
    void* result = malloc(suggested_size);
    return uv_buf_init((char*)result, suggested_size);
}

void init_settings() {
    settings.on_message_begin = on_message_begin;
    settings.on_url = on_url;
    settings.on_status_complete = on_status_complete;
    settings.on_header_field = on_header_field;
    settings.on_header_value = on_header_value;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
}

void HttpRequest::trace(const std::string& msg) {
    std::cerr << msg << std::endl;
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
    uv_close((uv_handle_t*)&_handle, on_closed);
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
        if (err.code == UV_EOF) {
            close();
        } else {
            fatal_error("on_request_read", uv_strerror(err));
        }
    } else {
        // It's normal for nread == 0, it's when uv requests a buffer then
        // decides it doesn't need it after all
    }

    free(buf.base);
}

void HttpRequest::handleRequest() {
    int r = uv_read_start((uv_stream_t*)&_handle, &on_alloc, &on_request_read);
    if (r) {
        uv_err_t err = uv_last_error(_pLoop);
        fatal_error("read_start", uv_strerror(err));
        return;
    }
}

void HttpRequest::onWrite(int status) {
    std::cerr << "Response written: " << status << std::endl;
}


#define IMPLEMENT_CALLBACK_1(function_name, return_type, type_1) \
    return_type function_name(type_1 arg1) { \
        return ((HttpRequest*)(arg1->data))->_##function_name(arg1); \
    }
#define IMPLEMENT_CALLBACK_2(function_name, return_type, type_1, type_2) \
    return_type function_name(type_1 arg1, type_2 arg2) { \
        return ((HttpRequest*)(arg1->data))->_##function_name(arg1, arg2); \
    }
#define IMPLEMENT_CALLBACK_3(function_name, return_type, type_1, type_2, type_3) \
    return_type function_name(type_1 arg1, type_2 arg2, type_3 arg3) { \
        return ((HttpRequest*)(arg1->data))->_##function_name(arg1, arg2, arg3); \
    }

IMPLEMENT_CALLBACK_1(on_message_begin, int, http_parser*)
IMPLEMENT_CALLBACK_3(on_url, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(on_status_complete, int, http_parser*)
IMPLEMENT_CALLBACK_3(on_header_field, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_3(on_header_value, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(on_headers_complete, int, http_parser*)
IMPLEMENT_CALLBACK_3(on_body, int, http_parser*, const char*, size_t)
IMPLEMENT_CALLBACK_1(on_message_complete, int, http_parser*)
IMPLEMENT_CALLBACK_1(on_closed, void, uv_handle_t*)
IMPLEMENT_CALLBACK_3(on_request_read, void, uv_stream_t*, ssize_t, uv_buf_t)

void on_close(uv_handle_t* handle) {
	printf("on_close\n");
	free(handle);
}

void on_connect(uv_stream_t* handle, int status) {
	assert(handle == (uv_stream_t*)&server);
	printf("on_connect\n");

    if (status == -1) {
        uv_err_t err = uv_last_error(handle->loop);
        fprintf(stderr, "connection error: %s\n", uv_strerror(err));
        return;
    }

    HttpRequest* req = new HttpRequest(handle->loop);

    fprintf(stdout, "Client handle is %p\n", req->handle());

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

	resbuf = uv_buf_init(const_cast<char*>(RESPONSE), sizeof(RESPONSE));

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
