#include "thread.h"

static uv_thread_t __main_thread__;
static uv_thread_t __background_thread__;

void register_main_thread() {
  __main_thread__ = uv_thread_self();
}

void register_background_thread() {
  __background_thread__ = uv_thread_self();
}

bool is_main_thread() {
  uv_thread_t cur_thread = uv_thread_self();
  return uv_thread_equal(&cur_thread, &__main_thread__);
}

bool is_background_thread() {
  uv_thread_t cur_thread = uv_thread_self();
  return uv_thread_equal(&cur_thread, &__background_thread__);
}
