#ifndef THREAD_HPP
#define THREAD_HPP

#include <uv.h>

// These must be called from the main and background thread, respectively, so
// that is_main_thread() and is_background_thread() can be tested later.
void register_main_thread();
void register_background_thread();

bool is_main_thread();
bool is_background_thread();


#ifdef DEBUG_THREAD
#include <assert.h>

// This must be called from the main thread so that thread assertions can be
// tested later.
#define ASSERT_MAIN_THREAD()         assert(is_main_thread());
#define ASSERT_BACKGROUND_THREAD()   assert(is_background_thread());

#else
#define ASSERT_MAIN_THREAD()
#define ASSERT_BACKGROUND_THREAD()

#endif // DEBUG_THREAD


#endif
