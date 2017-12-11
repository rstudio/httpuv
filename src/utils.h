#ifndef UTILS_H
#define UTILS_H

// A callback for deleting objects on the main thread using later(). This is
// needed when the object is an Rcpp object or contains one, because deleting
// such objects invoke R's memory management functions.
template <typename T>
void delete_cb_main(void* obj) {
  ASSERT_MAIN_THREAD()
  // later() expects a void*, so we have to cast it.
  T typed_obj = reinterpret_cast<T>(obj);
  delete typed_obj;
}

// Does the same as delete_cb_main, but checks that it's running on the
// background thread (when thread debugging is enabled).
template <typename T>
void delete_cb_bg(void* obj) {
  ASSERT_BACKGROUND_THREAD()
  T typed_obj = reinterpret_cast<T>(obj);
  delete typed_obj;
}


#endif
