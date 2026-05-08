sendWSMessage <- function(conn, binary, message) {
  invisible(.Call(C_sendWSMessage, conn, binary, message))
}

closeWS <- function(conn, code, reason) {
  invisible(.Call(C_closeWS, conn, as.integer(code), reason))
}

makeTcpServer <- function(host, port, onHeaders, onBodyData, onRequest,
                          onWSOpen, onWSMessage, onWSClose,
                          staticPaths, staticPathOptions, quiet) {
  .Call(C_makeTcpServer, host, as.integer(port), onHeaders, onBodyData,
        onRequest, onWSOpen, onWSMessage, onWSClose,
        staticPaths, staticPathOptions, quiet)
}

makePipeServer <- function(name, mask, onHeaders, onBodyData, onRequest,
                           onWSOpen, onWSMessage, onWSClose,
                           staticPaths, staticPathOptions, quiet) {
  .Call(C_makePipeServer, name, as.integer(mask), onHeaders, onBodyData,
        onRequest, onWSOpen, onWSMessage, onWSClose,
        staticPaths, staticPathOptions, quiet)
}

stopServer_ <- function(handle) {
  invisible(.Call(C_stopServer_, handle))
}

getStaticPaths_ <- function(handle) {
  .Call(C_getStaticPaths_, handle)
}

setStaticPaths_ <- function(handle, sp) {
  .Call(C_setStaticPaths_, handle, sp)
}

removeStaticPaths_ <- function(handle, paths) {
  .Call(C_removeStaticPaths_, handle, paths)
}

getStaticPathOptions_ <- function(handle) {
  .Call(C_getStaticPathOptions_, handle)
}

setStaticPathOptions_ <- function(handle, opts) {
  .Call(C_setStaticPathOptions_, handle, opts)
}

rawToBase64 <- function(x) {
  .Call(C_base64encode, x)
}

encodeURI <- function(value) {
  .Call(C_encodeURI, value)
}

encodeURIComponent <- function(value) {
  .Call(C_encodeURIComponent, value)
}

decodeURI <- function(value) {
  .Call(C_decodeURI, value)
}

decodeURIComponent <- function(value) {
  .Call(C_decodeURIComponent, value)
}

ipFamily <- function(ip) {
  .Call(C_ipFamily, ip)
}

invokeCppCallback <- function(data, callback_xptr) {
  invisible(.Call(C_invokeCppCallback, data, callback_xptr))
}

getRNGState <- function() {
  invisible(.Call(C_getRNGState))
}

wsconn_address <- function(external_ptr) {
  .Call(C_wsconn_address, external_ptr)
}

log_level <- function(level) {
  .Call(C_log_level, level)
}
