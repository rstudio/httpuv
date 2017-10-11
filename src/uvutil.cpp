#include "uvutil.h"
#include <string.h>

void throwError(int err,
  const std::string& prefix,
  const std::string& suffix) {

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

uint64_t InMemoryDataSource::size() const {
  return _buffer.size();
}
uv_buf_t InMemoryDataSource::getData(size_t bytesDesired) {
  size_t bytes = _buffer.size() - _pos;
  if (bytesDesired < bytes)
    bytes = bytesDesired;

  uv_buf_t mem;
  mem.base = bytes > 0 ? reinterpret_cast<char*>(&_buffer[_pos]) : 0;
  mem.len = bytes;

  _pos += bytes;
  return mem;
}
void InMemoryDataSource::freeData(uv_buf_t buffer) {
}
void InMemoryDataSource::close() {
  _buffer.clear();
}

void InMemoryDataSource::add(const std::vector<uint8_t>& moreData) {
  if (_buffer.capacity() < _buffer.size() + moreData.size())
    _buffer.reserve(_buffer.size() + moreData.size());
  _buffer.insert(_buffer.end(), moreData.begin(), moreData.end());
}

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
