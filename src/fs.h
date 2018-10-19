#ifndef FS_H
#define FS_H

#include <string>

#ifdef _WIN32
#else
#include <sys/stat.h>
#endif

// ============================================================================
// Filesystem-related functions
// ============================================================================

// Given a path, return just the filename part.
inline std::string basename(const std::string &path) {
  // TODO: handle Windows separators
  size_t found_idx = path.find_last_of('/');

  if (found_idx == std::string::npos) {
    return path;
  } else {
    return path.substr(found_idx + 1);
  }
}

// Given a filename, return the extension.
inline std::string find_extension(const std::string &filename) {
  size_t found_idx = filename.find_last_of('.');

  if (found_idx <= 0) {
    return "";
  } else {
    return filename.substr(found_idx + 1);
  }
}

inline bool is_directory(const std::string &filename) {
#ifdef _WIN32

  // TODO: implement

#else

  struct stat sb;

  if (stat(filename.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
    return true;
  } else {
    return false;
  }

#endif
}


#endif
