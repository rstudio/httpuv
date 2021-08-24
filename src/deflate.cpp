#include "deflate.h"
#include <stdexcept>
#include "utils.h"

namespace deflator {

typedef int(*flate_func)(z_stream* strm, int flush);

// Generic function for driving I/O loop for inflate/deflate
int flate(flate_func func, z_stream* strm, const char* data, size_t data_len, std::vector<char>& output) {

  strm->next_in = reinterpret_cast<unsigned char*>(const_cast<char*>(data));
  strm->avail_in = data_len;

  size_t orig_output_size = output.size();

  while (strm->avail_in) {
    // Ensure enough room is allocated on the output to receive 1024 more bytes
    const size_t CHUNK_SIZE = 1024;
    output.resize(output.size() + CHUNK_SIZE);

    strm->next_out = reinterpret_cast<unsigned char*>(&output[output.size() - CHUNK_SIZE]);
    strm->avail_out = CHUNK_SIZE;

    int error = func(strm, Z_SYNC_FLUSH);
    if (error != Z_OK && error != Z_STREAM_END) {
      // std::cerr << strm->msg << "\n";
      return error;
    }
  }
  size_t bytes_written = strm->total_out;
  size_t bytes_allocated = output.size() - orig_output_size;
  size_t bytes_to_erase = bytes_allocated - bytes_written;
  if (bytes_to_erase > 0) {
    output.erase(output.end() - bytes_to_erase, output.end());
  }

  return Z_OK;
}

Deflator::Deflator() {
  _stream = {0};
  _state = DeflatorStateStart;
}

Deflator::~Deflator() {
  if (_state == DeflatorStateReady) {
    int error = deflateEnd(&_stream);
    if (error != Z_OK) {
      debug_log("deflateEnd failed", LOG_WARN);
    }
  }
  _state = DeflatorStateEnded;
}

int Deflator::init(DeflateMode mode, int level, int windowBits, int memLevel, int strategy) {
  if (_state != DeflatorStateStart) {
    throw std::runtime_error("Deflator.init() was called twice");
  }

  if (windowBits < 9 || windowBits > 15) {
    throw std::runtime_error("Deflator.init() expects 9 <= windowBits <= 15");
  }

  if (mode == DeflateModeRaw) {
    windowBits *= -1;
  } else if (mode == DeflateModeGzip) {
    windowBits += 16;
  }

  int result = deflateInit2(&_stream, level, Z_DEFLATED, windowBits, memLevel, strategy);
  if (result == Z_OK) {
    _state = DeflatorStateReady;
  }
  return result;
}

int Deflator::deflate(const char* data, size_t data_len, std::vector<char>& output) {
  if (_state != DeflatorStateReady) {
    throw std::runtime_error("Deflator.init() must be called before deflate()");
  }
  return flate(::deflate, &_stream, data, data_len, output);
}

Inflator::Inflator() {
  _stream = {0};
  _state = DeflatorStateStart;
}

Inflator::~Inflator() {
  if (_state == DeflatorStateReady) {
    int error = inflateEnd(&_stream);
    if (error != Z_OK) {
      debug_log("inflateEnd failed", LOG_WARN);
    }
  }
  _state = DeflatorStateEnded;
}

int Inflator::init(DeflateMode mode, int windowBits) {
  if (_state != DeflatorStateStart) {
    throw std::runtime_error("Inflator.init() was called twice");
  }

  if (windowBits < 8 || windowBits > 15) {
    throw std::runtime_error("Inflator.init() expects 9 <= windowBits <= 15");
  }

  if (mode == DeflateModeRaw) {
    windowBits *= -1;
  } else if (mode == DeflateModeGzip) {
    windowBits += 16;
  }

  int result = inflateInit2(&_stream, windowBits);
  if (result == Z_OK) {
    _state = DeflatorStateReady;
  }
  return result;
}

int Inflator::inflate(const char* data, size_t data_len, std::vector<char>& output) {
  if (_state != DeflatorStateReady) {
    throw std::runtime_error("Inflator.init() must be called before deflate()");
  }
  return flate(::inflate, &_stream, data, data_len, output);
}

} // namespace
