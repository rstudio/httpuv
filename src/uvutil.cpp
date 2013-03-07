#include "uvutil.h"
#include <Rcpp/exceptions.h>

void throwLastError(uv_loop_t* pLoop,
  const std::string& prefix,
  const std::string& suffix) {

  uv_err_t err = uv_last_error(pLoop);
  std::string msg = prefix + uv_strerror(err) + suffix;
  throw Rcpp::exception(msg.c_str());
}


class WriteOp {
public:
  uv_write_t handle;
  ExtendedWrite* pParent;
  uv_buf_t buffer;

  WriteOp(ExtendedWrite* parent, uv_buf_t data)
        : pParent(parent), buffer(data) {
    memset(&handle, 0, sizeof(uv_write_t));
    handle.data = this;
  }

  void end() {
    pParent->_pDataSource->freeData(buffer);
    pParent->_activeWrites--;

    if (handle.handle->write_queue_size == 0) {
      // Write queue is empty, so we're ready to check for
      // more data and send if it available.
      pParent->next();
    }

    delete this;
  }
};

static void writecb(uv_write_t* handle, int status) {
  WriteOp* pWriteOp = (WriteOp*)handle->data;
  pWriteOp->end();
}

void ExtendedWrite::begin() {
  next();
}

void ExtendedWrite::next() {
  if (_errored) {
    if (_activeWrites == 0) {
      _pDataSource->close();
      onWriteComplete(1);
    }
    return;
  }

  uv_buf_t buf;
  try {
    buf = _pDataSource->getData(65536);
  } catch (Rcpp::exception e) {
    _errored = true;
    if (_activeWrites == 0) {
      _pDataSource->close();
      onWriteComplete(1);
    }
    return;
  }
  if (buf.len == 0) {
    _pDataSource->freeData(buf);
    if (_activeWrites == 0) {
      _pDataSource->close();
      onWriteComplete(0);
    }
    return;
  }
  WriteOp* pWriteOp = new WriteOp(this, buf);
  uv_write(&pWriteOp->handle, _pHandle, &pWriteOp->buffer, 1, &writecb);
  _activeWrites++;
  //uv_write(pReq)
}
