#ifndef DEBUG_HPP
#define DEBUG_HPP


// Uncomment this line to enable extra checks that functions are being called
// from the correct thread.
#define DEBUG_THREAD

#ifdef DEBUG_THREAD
#include <assert.h>
#include <uv.h>

// So that assert() works
#undef NDEBUG

extern uv_thread_t __main_thread__;

// This must be called from the main thread so that thread assertions can be
// tested later.
#define REGISTER_MAIN_THREAD()     __main_thread__ = uv_thread_self();
#define ASSERT_MAIN_THREAD()       assert(uv_thread_self() == __main_thread__);
#define ASSERT_BACKGROUND_THREAD() assert(uv_thread_self() != __main_thread__);

#else
#define REGISTER_MAIN_THREAD()
#define ASSERT_MAIN_THREAD()
#define ASSERT_BACKGROUND_THREAD()

#endif // DEBUG_THREAD


// Uncomment this line to print out tracing messages.
// #define DEBUG_TRACE
#include <string>
void trace(const std::string& msg);


#endif
