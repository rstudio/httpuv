#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <uv.h>
#include <http_parser.h>
#include <boost/bind/bind.hpp>


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

void on_close(uv_handle_t* handle) {
	printf("on_close\n");
	free(handle);
}

void on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf) {
	client_t* client = (client_t*)stream->data;

	if (nread >= 0) {
		int parsed = http_parser_execute(&client->parser, &settings, buf.base, nread);
		if (parsed < nread) {
			fprintf(stderr, "parse error\n");
			uv_close((uv_handle_t*)stream, &on_close);
		}
	}
	else {
		uv_err_t err = uv_last_error(uv_default_loop());
		if (err.code == UV_EOF) {
			uv_close((uv_handle_t*)stream, &on_close);
		}
		else {
			fprintf(stderr, "accept: %s\n", uv_strerror(err));
		}
	}

	free(buf.base);
}

void on_connect(uv_stream_t* handle, int status) {
	assert(handle == (uv_stream_t*)&server);
	printf("on_connect\n");

	client_t* client = (client_t*)malloc(sizeof(client_t));
	uv_tcp_init(uv_default_loop(), &client->handle);
	fprintf(stdout, "Client is %p\n", client);
	fprintf(stdout, "Client handle is %p\n", &client->handle);
	http_parser_init(&client->parser, HTTP_REQUEST);
	client->parser.data = client;
	client->handle.data = client;

	int r = uv_accept(handle, (uv_stream_t*)&client->handle);
	if (r) {
		uv_err_t err = uv_last_error(uv_default_loop());
		fprintf(stderr, "accept: %s\n", uv_strerror(err));
		return;
	}

	r = uv_read_start((uv_stream_t*)&client->handle, &on_alloc, &on_read);
	if (r) {
		uv_err_t err = uv_last_error(uv_default_loop());
		fprintf(stderr, "read: %s\n", uv_strerror(err));
		return;
	}
}

void on_close_client(uv_handle_t* client_handle) {
	client_t* client = (client_t*)client_handle->data;
	free(client);
}

void after_write(uv_write_t* req, int status) {
	client_t* client = (client_t*)req->data;
	uv_close((uv_handle_t*)&client->handle, &on_close_client);
}

int on_headers_complete(http_parser* parser) {
	client_t* client = (client_t*)parser->data;

	client->write_req.data = client;
	uv_write((uv_write_t*)&client->write_req, (uv_stream_t*)&client->handle,
		&resbuf, 1, &after_write);

	return 1;
}

int main() {

	resbuf = uv_buf_init(const_cast<char*>(RESPONSE), sizeof(RESPONSE));

	settings.on_headers_complete = on_headers_complete;

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

