#ifndef _WIN32

#include "filedatasource.h"
#include "utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int FileDataSource::initialize(const std::string& path, bool owned) {
  // This can be called from either the main thread or background thread.

  _fd = open(path.c_str(), O_RDONLY);
  if (_fd == -1) {
    _lastErrorMessage = "Error opening file: " + toString(errno) + "\n";
    return 1;
  }
  else {
    struct stat info = {0};
    if (fstat(_fd, &info)) {
      _lastErrorMessage = "Error opening path: " + toString(errno) + "\n";
      ::close(_fd);
      return 1;
    }
    _length = info.st_size;

    if (owned && unlink(path.c_str())) {
      // Print this (on either main or background thread), since we're not
      // returning 1 to indicate an error.
      err_printf("Couldn't delete temp file: %d\n", errno);
      // It's OK to continue
    }

    return 0;
  }
}

uint64_t FileDataSource::size() const {
  return _length;
}

uv_buf_t FileDataSource::getData(size_t bytesDesired) {
  ASSERT_BACKGROUND_THREAD()
  if (bytesDesired == 0)
    return uv_buf_init(NULL, 0);

  char* buffer = (char*)malloc(bytesDesired);
  if (!buffer) {
    throw std::runtime_error("Couldn't allocate buffer");
  }

  ssize_t bytesRead = read(_fd, buffer, bytesDesired);
  if (bytesRead == -1) {
    err_printf("Error reading: %d\n", errno);
    free(buffer);
    throw std::runtime_error("File read failed");
  }

  return uv_buf_init(buffer, bytesRead);
}

void FileDataSource::freeData(uv_buf_t buffer) {
  free(buffer.base);
}

void FileDataSource::close() {
  if (_fd != -1)
    ::close(_fd);
  _fd = -1;
}

std::string FileDataSource::lastErrorMessage() const {
  return _lastErrorMessage;
}


#endif // #ifndef _WIN32
