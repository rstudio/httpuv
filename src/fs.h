#ifndef FS_H
#define FS_H

#include <string>

std::string basename(const std::string &path);

std::string find_extension(const std::string &filename);

bool is_directory(const std::string &filename);

#endif
