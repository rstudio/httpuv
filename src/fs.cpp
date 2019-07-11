#include "fs.h"
#include "utils.h"

#ifdef _WIN32
#include <windows.h>
#include "winutils.h"
#else
#include <sys/stat.h>
#endif

#include <iostream>


// Given a filename, return the extension.
std::string find_extension(const std::string &filename) {
  size_t found_idx = filename.find_last_of('.');

  if (found_idx <= 0) {
    return "";
  } else {
    return filename.substr(found_idx + 1);
  }
}

// Given a filename, return the extension.
std::string basename(const std::string &path) {
  // TODO: handle Windows separators
  size_t found_idx = path.find_last_of('/');

  if (found_idx == std::string::npos) {
    return path;
  } else {
    return path.substr(found_idx + 1);
  }
}


// filename is assumed to be UTF-8.
bool is_directory(const std::string &filename) {
#ifdef _WIN32

  DWORD file_attr = GetFileAttributesW(utf8ToWide(filename).data());
  if (file_attr == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  if (file_attr & FILE_ATTRIBUTE_DIRECTORY) {
    return true;
  }

  return false;

#else

  struct stat sb;

  if (stat(filename.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
    return true;
  } else {
    return false;
  }

#endif
}
