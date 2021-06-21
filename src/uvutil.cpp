#include "uvutil.h"
#include "thread.h"
#include <string.h>


class WriteOp {
private:
  ExtendedWrite* pParent;
  uv_buf_t buffer;
  // Bytes to write before writing the buffer
  std::vector<char> prefix;
  // Bytes to write after writing the buffer
  std::vector<char> suffix;
public:
  uv_write_t handle;

  WriteOp(ExtendedWrite* parent, uv_buf_t data)
        : pParent(parent), buffer(data) {
    memset(&handle, 0, sizeof(uv_write_t));
    handle.data = this;
  }

  void setPrefix(const char* data, size_t len) {
    ASSERT_BACKGROUND_THREAD()
    prefix.clear();
    std::copy(data, data + len, std::back_inserter(prefix));
  }

  void setSuffix(const char* data, size_t len) {
    ASSERT_BACKGROUND_THREAD()
    suffix.clear();
    std::copy(data, data + len, std::back_inserter(suffix));
  }

  std::vector<uv_buf_t> bufs() {
    ASSERT_BACKGROUND_THREAD()
    std::vector<uv_buf_t> res;
    if (prefix.size() > 0) {
      res.push_back(uv_buf_init(&prefix[0], prefix.size()));
    }
    res.push_back(buffer);
    if (suffix.size() > 0) {
      res.push_back(uv_buf_init(&suffix[0], suffix.size()));
    }
    return res;
  }

  void end() {
    ASSERT_BACKGROUND_THREAD()
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
  ASSERT_BACKGROUND_THREAD()
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
  ASSERT_BACKGROUND_THREAD()
  _buffer.clear();
}

void InMemoryDataSource::add(const std::vector<uint8_t>& moreData) {
  ASSERT_BACKGROUND_THREAD()
  if (_buffer.capacity() < _buffer.size() + moreData.size())
    _buffer.reserve(_buffer.size() + moreData.size());
  _buffer.insert(_buffer.end(), moreData.begin(), moreData.end());
}

static void writecb(uv_write_t* handle, int status) {
  ASSERT_BACKGROUND_THREAD()
  WriteOp* pWriteOp = (WriteOp*)handle->data;
  pWriteOp->end();
}

void ExtendedWrite::begin() {
  ASSERT_BACKGROUND_THREAD()
  next();
}

const std::string CRLF = "\r\n";
const std::string TRAILER = "0\r\n\r\n";

void ExtendedWrite::next() {
  ASSERT_BACKGROUND_THREAD()
  if (_errored) {
    if (_activeWrites == 0) {
      _pDataSource->close();
      onWriteComplete(1);
    }
    return;
  }
  if (_completed) {
    if (_activeWrites == 0) {
      _pDataSource->close();
      onWriteComplete(0);
    }
    return;
  }

  uv_buf_t buf;
  try {
    buf = _pDataSource->getData(65536);
  } catch (std::exception& e) {
    _errored = true;
    if (_activeWrites == 0) {
      _pDataSource->close();
      onWriteComplete(1);
    }
    return;
  }
  if (buf.len == 0) {
    // No more data is going to come.
    // Ensure future calls to next() results in disposal (assuming that all
    // outstanding writes are done).
    _completed = true;
  }

  WriteOp* pWriteOp = new WriteOp(this, buf);
  if (this->_chunked) {
    if (buf.len == 0) {
      // In chunked mode, the last chunk must be followed by one more "\r\n".
      pWriteOp->setSuffix(TRAILER.c_str(), TRAILER.length());
    } else {
      // In chunked mode, data chunks must be preceded by 1) the number of bytes
      // in the chunk, as a hexadecimal string; and 2) "\r\n"; and succeeded by
      // another "\r\n"
      char prefix[16] = {0};
      int len = snprintf(prefix, sizeof(prefix), "%lX\r\n", buf.len);
      pWriteOp->setPrefix(prefix, len);
      pWriteOp->setSuffix(CRLF.c_str(), CRLF.length());
    }
  } else {
    // Non-chunked mode
    if (buf.len == 0) {
      // We've reached the end of the response body. Technically there's nothing
      // to uv_write; we're passing a uv_buf_t, but it's actually empty. The
      // reason we're doing it anyway is to avoid having two cleanup paths, one
      // for chunked and one for non-chunked; it's hard enough to reason about
      // as it is.
    } else {
      // This is the simple/common case; we're about to write some data to the
      // socket, then we'll come back and see if there's more to write.
    }
  }
  _activeWrites++;
  auto op_bufs = pWriteOp->bufs();
  uv_write(&pWriteOp->handle, _pHandle, &op_bufs[0], op_bufs.size(), &writecb);
}
