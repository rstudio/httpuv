#include "rcpp_hello_world.h"
#include <stdio.h>
#include <uv.h>

int64_t counter = 0;

void wait_for_a_while(uv_idle_t* handle, int status) {
    counter++;

    if (counter >= 10e6)
        uv_idle_stop(handle);
}

SEXP rcpp_hello_world(){
    using namespace Rcpp ;
    
    CharacterVector x = CharacterVector::create( "foo", "bar" )  ;
    NumericVector y   = NumericVector::create( 0.0, 1.0 ) ;
    List z            = List::create( x, y ) ;
    


    uv_idle_t idler;

    uv_idle_init(uv_default_loop(), &idler);
    uv_idle_start(&idler, wait_for_a_while);

    printf("Idling...\n");
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    
    return z ;
}
