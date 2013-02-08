#include "rcpp_hello_world.h"
#include <stdio.h>
#include <string>
#include <uv.h>
#include "webserver.hpp"

class RRequestHandler : public RequestHandler {
private:
    Rcpp::Function* _pFunc;

public:
    RRequestHandler(Rcpp::Function* pFunc) {
        _pFunc = pFunc;
    }

    virtual ~RRequestHandler() {
        delete _pFunc;
    }

    virtual HttpResponse* getResponse(HttpRequest* pRequest) {
        using namespace Rcpp;
        Environment env;
        env["REQUEST_METHOD"] = pRequest->method();
        env["URL"] = pRequest->url();

        // TODO: Fix memory leak!
        std::string* pResult = new std::string(Rcpp::as<std::string>((*_pFunc)(env)));
        return new HttpResponse(pRequest, 200, "OK", const_cast<char*>(pResult->c_str()), pResult->size());
    }
};

// [[Rcpp::export]]
intptr_t makeServer(Rcpp::Function func){
    using namespace Rcpp;
    Function* pFunc = new Function(func);
    RRequestHandler* pHandler = new RRequestHandler(pFunc);
    uv_tcp_t* pServer = createServer(
        uv_default_loop(), "0.0.0.0", 8000, (RequestHandler*)pHandler);

    return (intptr_t)pServer;
}

// [[Rcpp::export]]
void destroyServer(intptr_t handle) {
    freeServer((uv_tcp_t*)handle);
}

// [[Rcpp::export]]
bool run() {
    return runNonBlocking(uv_default_loop());
}