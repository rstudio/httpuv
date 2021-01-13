#ifndef FILEDATASOURCE_H
#define FILEDATASOURCE_H

#include "uvutil.h"


// Status codes for FileDataSource::initialize().
enum FileDataSourceResult {
  FDS_OK = 0,     // Initialization worked
  FDS_NOT_EXIST,    // File did not exist
  FDS_ISDIR,      // File is a directory
  FDS_ERROR       // Other error
};


class FileDataSource : public DataSource {
#ifdef _WIN32
  HANDLE _hFile;
  uint64_t _payloadSize;
  LARGE_INTEGER _fsize;
#else
  int _fd;
  off_t _payloadSize;
  off_t _fsize;
#endif
  std::string _lastErrorMessage;

public:
  FileDataSource() {}

  ~FileDataSource() {
    close();
  }

  FileDataSourceResult initialize(const std::string& path, bool owned);
  // Total size of the data in this source, in bytes, after accounting for file
  // size and range settings.
  uint64_t size() const;
  // Total size of the underlying file in this source, in bytes.
  uint64_t fileSize() const;
  bool setRange(uint64_t start, uint64_t end);
  uv_buf_t getData(size_t bytesDesired);
  void freeData(uv_buf_t buffer);
  // Get the mtime of the file. If there's an error, return 0.
  time_t getMtime();
  void close();
  std::string lastErrorMessage() const;
};

#endif // FILEDATASOURCE_H
