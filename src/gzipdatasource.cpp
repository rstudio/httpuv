#include "gzipdatasource.h"
#include "utils.h"

GZipDataSource::GZipDataSource(std::shared_ptr<DataSource> pData) :
  _pData(pData), _state(Streaming) {

  _zstrm = {0};
  int res = deflateInit2(&_zstrm, 4, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
  if (res != Z_OK) {
    if (_zstrm.msg) {
      throw std::runtime_error(_zstrm.msg);
    } else {
      throw std::runtime_error("zlib initialization failed");
    }
  }
}

GZipDataSource::~GZipDataSource() {
  freeInputBuffer(true);
  // ignore errors on destruction
  deflateEnd(&_zstrm);
}

uint64_t GZipDataSource::size() const {
  debug_log("GZipDataSource::size() was called, this should never happen\n", LOG_WARN);
  return 0;
}

uv_buf_t GZipDataSource::getData(size_t bytesDesired) {
  if (_state == Done) {
    // GZip stream written, nothing more to do
    return {0};
  }

  // Prepare the output area to be written to
  Bytef* outputBuf = (Bytef*)malloc(bytesDesired);
  _zstrm.next_out = outputBuf;
  _zstrm.avail_out = bytesDesired;

  // There's room to write, and things we need to write: if Streaming, then
  // there's potentially more data; and if Finishing, then we need to write
  // the gzip footer.
  while (_zstrm.avail_out > 0 && _state != Done) {
    if (_state == Streaming && _zstrm.avail_in == 0) {
      freeInputBuffer();

      _inputBuf = _pData->getData(bytesDesired);
      _zstrm.next_in = (Bytef*)_inputBuf.base;
      _zstrm.avail_in = _inputBuf.len;

      if (_inputBuf.len == 0) {
        _state = Finishing;
      }
    }

    deflateNext();
  }

  freeInputBuffer();

  uv_buf_t ret = {0};
  ret.base = (char*)outputBuf;
  ret.len = bytesDesired - _zstrm.avail_out;
  return ret;
}

void GZipDataSource::freeData(uv_buf_t buffer) {
  free(buffer.base);
}

void GZipDataSource::close() {
  _pData->close();
}

// Attempt to deflate more data, reading from _zstrm.next_in and writing to
// _zstrm.next_out. Both reads and (potentially) writes _state.
void GZipDataSource::deflateNext() {
  int res = deflate(&_zstrm, (_state == Finishing) ? Z_FINISH : Z_NO_FLUSH);
  if (res == Z_STREAM_END) {
    _state = Done;
  } else if (res != Z_OK) {
    throw std::runtime_error("deflate failed!");
  }
}

// Use force=true to free the buffer even if _zstrm might still be using it
bool GZipDataSource::freeInputBuffer(bool force) {
  if ((force || _zstrm.avail_in == 0) && _inputBuf.base) {
    _pData->freeData(_inputBuf);
    _inputBuf = {0};
    _zstrm.next_in = Z_NULL;
    _zstrm.avail_in = 0;
    return true;
  } else {
    return false;
  }
}
