#include "winutils.h"
#ifdef _WIN32

#include <windows.h>
#include <vector>

std::string wideToUtf8(const std::wstring& value)
{
   if (value.size() == 0)
      return std::string();

   const wchar_t * cstr = value.c_str();
   int chars = ::WideCharToMultiByte(CP_UTF8, 0,
                                     cstr, -1,
                                     nullptr, 0, nullptr, nullptr);
   if (chars == 0) {
      return std::string();
   }

   std::vector<char> result(chars, 0);
   chars = ::WideCharToMultiByte(CP_UTF8,
                                 0,
                                 cstr,
                                 -1,
                                 &(result[0]),
                                 result.size(),
                                 nullptr, nullptr);

   return std::string(&(result[0]));
}


std::wstring utf8ToWide(const std::string& value,
                        const std::string& context)
{
   if (value.size() == 0)
      return std::wstring();

   const char * cstr = value.c_str();
   int chars = ::MultiByteToWideChar(CP_UTF8, 0,
                                     cstr, -1,
                                     nullptr, 0);
   if (chars == 0) {
      return std::wstring();
   }

   std::vector<wchar_t> result(chars, 0);
   chars = ::MultiByteToWideChar(CP_UTF8, 0,
                                 cstr, -1,
                                 &(result[0]),
                                 result.size());

   return std::wstring(&(result[0]));
}

#endif // ifdef _WIN32
