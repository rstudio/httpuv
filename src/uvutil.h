#ifndef UVUTIL_HPP
#define UVUTIL_HPP

#include "thread.h"
#include <string>
#include <vector>
#include "libuv/include/uv.h"

/* Prevent naming conflicts for Free() and Calloc() */
#define R_NO_REMAP
// This could be defined from Makevars.win. In that case, don't re-define it.
#ifndef STRICT_R_HEADERS
#define STRICT_R_HEADERS
#endif
#include <Rcpp.h>

inline uv_handle_t* toHandle(uv_timer_t* timer) {
  return (uv_handle_t*)timer;
}
inline uv_handle_t* toHandle(uv_tcp_t* tcp) {
  return (uv_handle_t*)tcp;
}
inline uv_handle_t* toHandle(uv_stream_t* stream) {
  return (uv_handle_t*)stream;
}

inline uv_stream_t* toStream(uv_tcp_t* tcp) {
  return (uv_stream_t*)tcp;
}

class WriteOp;

// Abstract class for synchronously streaming known-length data
// without needing to know where the data comes from.
class DataSource {
public:
  virtual ~DataSource() {}
  virtual uint64_t size() const = 0;
  virtual uv_buf_t getData(size_t bytesDesired) = 0;
  virtual void freeData(uv_buf_t buffer) = 0;
  virtual void close() = 0;
};

class InMemoryDataSource : public DataSource {
private:
  std::vector<uint8_t> _buffer;
  size_t _pos;
public:
  explicit InMemoryDataSource(const std::vector<uint8_t>& buffer = std::vector<uint8_t>())
    : _buffer(buffer), _pos(0) {}

  explicit InMemoryDataSource(const Rcpp::RawVector& rawVector)
    : _buffer(rawVector.size()), _pos(0) 
  {
    ASSERT_MAIN_THREAD()
    std::copy(rawVector.begin(), rawVector.end(), _buffer.begin());
  }

  virtual ~InMemoryDataSource() {}
  uint64_t size() const;
  uv_buf_t getData(size_t bytesDesired);
  void freeData(uv_buf_t buffer);
  void close();

  void add(const std::vector<uint8_t>& moreData);
};

// Class for writing a DataSource to a uv_stream_t. Takes care
// not to buffer too much data in memory (happens when you try
// to write too much data to a slow uv_stream_t).
class ExtendedWrite {
  int _activeWrites;
  bool _errored;
  uv_stream_t* _pHandle;
  DataSource* _pDataSource;

public:
  ExtendedWrite(uv_stream_t* pHandle, DataSource* pDataSource)
      : _activeWrites(0), _errored(false), _pHandle(pHandle),
        _pDataSource(pDataSource) {}
  virtual ~ExtendedWrite() {}
  
  virtual void onWriteComplete(int status) = 0;

  void begin();
  friend class WriteOp;

protected:
  void next();

};


inline int ip_family(const std::string& ip) {
  // A buffer big enough for an IPv6 address
  unsigned char addr[16];

  if (uv_inet_pton(AF_INET6, ip.c_str(), addr) == 0) {
    return AF_INET6;
  }
  if (uv_inet_pton(AF_INET, ip.c_str(), addr) == 0) {
    return AF_INET;
  }
  return -1;
}

#endif // UVUTIL_HPP
