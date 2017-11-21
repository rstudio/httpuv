#ifdef _WIN32

#include "filedatasource.h"
#include <Windows.h>

// Windows gets a whole different implementation of FileDataSource
// so we can use FILE_FLAG_DELETE_ON_CLOSE, which is not available
// using the POSIX file functions.

int FileDataSource::initialize(const std::string& path, bool owned) {

  DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;
  if (owned)
    flags |= FILE_FLAG_DELETE_ON_CLOSE;

  _hFile = CreateFile(path.c_str(),
                      GENERIC_READ,
                      0, // exclusive access
                      NULL, // security attributes
                      OPEN_EXISTING,
                      flags,
                      NULL);

  if (_hFile == INVALID_HANDLE_VALUE) {
    REprintf("Error opening file: %d\n", GetLastError());
    return 1;
  }

  if (!GetFileSizeEx(_hFile, &_length)) {
    CloseHandle(_hFile);
    REprintf("Error retrieving file size: %d\n", GetLastError());
    return 1;
  }

  return 0;
}

uint64_t FileDataSource::size() const {
  return _length.QuadPart;
}

uv_buf_t FileDataSource::getData(size_t bytesDesired) {
  if (bytesDesired == 0)
    return uv_buf_init(NULL, 0);

  char* buffer = (char*)malloc(bytesDesired);
  if (!buffer) {
    throw std::runtime_error("Couldn't allocate buffer");
  }

  DWORD bytesRead;
  if (!ReadFile(_hFile, buffer, bytesDesired, &bytesRead, NULL)) {
    REprintf("Error reading: %d\n", GetLastError());
    free(buffer);
    throw std::runtime_error("File read failed");
  }

  return uv_buf_init(buffer, bytesRead);
}

void FileDataSource::freeData(uv_buf_t buffer) {
  free(buffer.base);
}

void FileDataSource::close() {
  if (_hFile != INVALID_HANDLE_VALUE) {
    CloseHandle(_hFile);
    _hFile = INVALID_HANDLE_VALUE;
  }
}

#endif // #ifdef _WIN32
