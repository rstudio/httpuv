#ifndef DEBUG_HPP
#define DEBUG_HPP

// See the Makevars file to see how to compile with various debugging settings.

#ifdef DEBUG_THREAD
#include <assert.h>
#include <uv.h>

extern uv_thread_t __main_thread__;
extern uv_thread_t __background_thread__;

// This must be called from the main thread so that thread assertions can be
// tested later.
#define REGISTER_MAIN_THREAD()       __main_thread__ = uv_thread_self();
#define REGISTER_BACKGROUND_THREAD() __background_thread__ = uv_thread_self();
#define ASSERT_MAIN_THREAD()         assert(uv_thread_self() == __main_thread__);
#define ASSERT_BACKGROUND_THREAD()   assert(uv_thread_self() == __background_thread__);

#else
#define REGISTER_MAIN_THREAD()
#define REGISTER_BACKGROUND_THREAD()
#define ASSERT_MAIN_THREAD()
#define ASSERT_BACKGROUND_THREAD()

#endif // DEBUG_THREAD


#include <string>
void trace(const std::string& msg);


#endif
