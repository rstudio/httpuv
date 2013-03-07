#ifndef FILEDATASOURCE_H
#define FILEDATASOURCE_H

#include "uvutil.h"

class FileDataSource : public DataSource {
#ifdef _WIN32
  HANDLE _hFile;
  LARGE_INTEGER _length;
#else
  int _fd;
  off_t _length;
#endif

public:
  FileDataSource() {}

  int initialize(const std::string& path, bool owned);
  uint64_t size() const;
  uv_buf_t getData(size_t bytesDesired);
  void freeData(uv_buf_t buffer);
  void close();
};

#endif // FILEDATASOURCE_H