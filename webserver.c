#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <map>
#include <string>

#include <uv.h>
#include <http_parser.h>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>


static uv_tcp_t server;
static http_parser_settings settings;
static uv_buf_t resbuf;

#define RESPONSE \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/plain\r\n" \
	"Content-Length: 12\r\n" \
	"\r\n" \
	"hello world\n"

typedef struct {
	uv_tcp_t handle;
	http_parser parser;
	uv_write_t write_req;
} client_t;

uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size) {
    void* result = malloc(suggested_size);
    return uv_buf_init((char*)result, suggested_size);
}

#define SHIM1(function_name, return_type, type_1) \
    return_type function_name(type_1 arg1);
#define SHIM3(function_name, return_type, type_1, type_2, type_3) \
    return_type function_name(type_1 arg1, type_2 arg2, type_3 arg3);

SHIM1(on_message_begin, int, http_parser*)
SHIM3(on_url, int, http_parser*, const char*, size_t)
SHIM1(on_status_complete, int, http_parser*)
SHIM3(on_header_field, int, http_parser*, const char*, size_t)
SHIM3(on_header_value, int, http_parser*, const char*, size_t)
SHIM1(on_headers_complete, int, http_parser*)
SHIM3(on_body, int, http_parser*, const char*, size_t)
SHIM1(on_message_complete, int, http_parser*)
SHIM1(on_closed, void, uv_handle_t*)
SHIM3(on_read, void, uv_stream_t*, ssize_t, uv_buf_t)


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

class HttpRequest : public boost::enable_shared_from_this<HttpRequest> {

private:
	uv_loop_t* _pLoop;
    uv_tcp_t _handle;
    http_parser _parser;
    std::map<std::string, std::string> _headers;
    std::string _lastHeaderField;
    unsigned long _bytesRead;

    void trace(const std::string& msg) {
        std::cerr << msg << std::endl;
    }

public:
    HttpRequest(uv_loop_t* pLoop) : _bytesRead(0) {
        _pLoop = pLoop;

        uv_tcp_init(pLoop, &_handle);
        _handle.data = this;

        http_parser_init(&_parser, HTTP_REQUEST);
        _parser.data = this;
    }

    virtual ~HttpRequest() {
    }

    uv_tcp_t* handle() {
        return &_handle;
    }

    virtual int _on_message_begin(http_parser* pParser) {
        trace("on_message_begin");
        _headers.clear();
        return 0;
    }
    virtual int _on_url(http_parser* pParser, const char* pAt, size_t length) {
        trace("on_url");
        return 0;
    }
    virtual int _on_status_complete(http_parser* pParser) {
        trace("on_status_complete");
        return 0;
    }
    virtual int _on_header_field(http_parser* pParser, const char* pAt, size_t length) {
        trace("on_header_field");
        _lastHeaderField = std::string(pAt, length);
        return 0;
    }
    virtual int _on_header_value(http_parser* pParser, const char* pAt, size_t length) {
        trace("on_header_value");
        _headers[_lastHeaderField] = std::string(pAt, length);
        return 0;
    }
    virtual int _on_headers_complete(http_parser* pParser) {
        trace("on_headers_complete");
        for (std::map<std::string, std::string>::iterator iter = _headers.begin();
             iter != _headers.end();
             iter++) {
            std::cout << iter->first << std::string(" = ") << iter->second << std::endl;
        }
        return 0;
    }
    virtual int _on_body(http_parser* pParser, const char* pAt, size_t length) {
        trace("on_body");
        _bytesRead += length;
        return 0;
    }
    virtual int _on_message_complete(http_parser* pParser) {
        trace("on_message_complete");
        std::cout << _bytesRead << " bytes read" << std::endl;
        _headers.clear();

        // TODO: Send response

        return 0;
    }

    void fatal_error(const char* method, const char* message) {
		fprintf(stderr, "ERROR: [%s] %s\n", method, message);
	}

    void _on_closed(uv_handle_t* handle) {
		printf("Closed\n");
	}

	void close() {
        uv_close((uv_handle_t*)&_handle, on_closed);
	}

    void execute(uv_stream_t* stream) {
    }

    void _on_read(uv_stream_t*, ssize_t nread, uv_buf_t buf) {
        if (nread > 0) {
            std::cerr << nread << " bytes read\n";
            int parsed = http_parser_execute(&_parser, &settings, buf.base, nread);
            if (_parser.upgrade) {
                // TODO: Handle HTTP upgrade
            } else if (parsed < nread) {
				fatal_error("on_read", "parse error");
                close();
			}
        } else if (nread < 0) {
			uv_err_t err = uv_last_error(_pLoop);
			if (err.code == UV_EOF) {
				close();
			} else {
				fatal_error("on_read", uv_strerror(err));
			}
		}

		free(buf.base);
	}

    void handleRequest() {
        int r = uv_read_start((uv_stream_t*)&_handle, &on_alloc, &on_read);
        if (r) {
            uv_err_t err = uv_last_error(_pLoop);
            fatal_error("read_start", uv_strerror(err));
            return;
        }

    }
};


#define SHIM1_IMPL(function_name, return_type, type_1) \
    return_type function_name(type_1 arg1) { \
        return ((HttpRequest*)(arg1->data))->_##function_name(arg1); \
    }
#define SHIM3_IMPL(function_name, return_type, type_1, type_2, type_3) \
    return_type function_name(type_1 arg1, type_2 arg2, type_3 arg3) { \
        return ((HttpRequest*)(arg1->data))->_##function_name(arg1, arg2, arg3); \
    }

SHIM1_IMPL(on_message_begin, int, http_parser*)
SHIM3_IMPL(on_url, int, http_parser*, const char*, size_t)
SHIM1_IMPL(on_status_complete, int, http_parser*)
SHIM3_IMPL(on_header_field, int, http_parser*, const char*, size_t)
SHIM3_IMPL(on_header_value, int, http_parser*, const char*, size_t)
SHIM1_IMPL(on_headers_complete, int, http_parser*)
SHIM3_IMPL(on_body, int, http_parser*, const char*, size_t)
SHIM1_IMPL(on_message_complete, int, http_parser*)
SHIM1_IMPL(on_closed, void, uv_handle_t*)
SHIM3_IMPL(on_read, void, uv_stream_t*, ssize_t, uv_buf_t)

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

void on_close_client(uv_handle_t* client_handle) {
	client_t* client = (client_t*)client_handle->data;
	free(client);
}

void after_write(uv_write_t* req, int status) {
	client_t* client = (client_t*)req->data;
	uv_close((uv_handle_t*)&client->handle, &on_close_client);
}

int on_headers_complete1(http_parser* parser) {
	client_t* client = (client_t*)parser->data;

	client->write_req.data = client;
	uv_write((uv_write_t*)&client->write_req, (uv_stream_t*)&client->handle,
		&resbuf, 1, &after_write);

	return 1;
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

