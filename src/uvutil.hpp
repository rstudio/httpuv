#ifndef UVUTIL_HPP
#define UVUTIL_HPP

#include <uv.h>

uv_handle_t* toHandle(uv_timer_t* timer) {
  return (uv_handle_t*)timer;
}
uv_handle_t* toHandle(uv_tcp_t* tcp) {
  return (uv_handle_t*)tcp;
}
uv_handle_t* toHandle(uv_stream_t* stream) {
  return (uv_handle_t*)stream;
}

uv_stream_t* toStream(uv_tcp_t* tcp) {
  return (uv_stream_t*)tcp;
}

#endif // UVUTIL_HPP