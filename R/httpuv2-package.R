#' @keywords internal
"_PACKAGE"

## usethis namespace: start
#' @useDynLib httpuv2, .registration=TRUE
## usethis namespace: end
NULL

## usethis namespace: start
#' @importFrom later2 promise then finally is.promise run_now
#' @importFrom R6 R6Class
## usethis namespace: end
NULL

# The following functions take C++ functions and provide an exported wrapper.
# The approach is
# R: myfun(); C++: myfun_().
# The C++ functions are registered with cpp11, and the R functions are exported.
# 'cpp4' (not in use) "saves" this step by allowing this on C++ side:
# /* roxygen
# @title My Function
# @param x something.
# @export
# */
# int myfun(int x) {
#   return x + 1;
# }
