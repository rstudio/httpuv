#ifndef WINUTILS_H
#define WINUTILS_H
#ifdef _WIN32

#include <string>

std::string wideToUtf8(const std::wstring& value);

std::wstring utf8ToWide(const std::string& value,
                        const std::string& context = std::string());

#endif // #ifdef _WIN32
#endif // #define WINUTILS_H
