// This resolves a conflict between R.h and windows.h.
//
// It's very important that R.h (or Rcpp.h) is included first,
// fixup.h is included next, and windows.h (or uv.h) is included
// after that, otherwise the build will fail on Windows.
#undef Realloc
#undef Free
