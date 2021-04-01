#ifdef _WIN32
#include "timegm.h"

#include <errno.h>
#include <timezoneapi.h>

time_t timegm(struct tm *tm) {
  SYSTEMTIME st = {0};
  st.wYear = tm->tm_year + 1900;
  st.wMonth = tm->tm_mon + 1;
  st.wDay = tm->tm_mday;
  st.wHour = tm->tm_hour;
  st.wMinute = tm->tm_min;
  st.wSecond = tm->tm_sec;
  st.wDayOfWeek = tm->tm_wday;

  FILETIME ft = {0};
  if (!SystemTimeToFileTime(&st, &ft)) {
    errno = EOVERFLOW;
    return -1;
  }

  ULARGE_INTEGER res;
  res.LowPart = ft.dwLowDateTime;
  res.HighPart = ft.dwHighDateTime;
  return res.QuadPart / 10000000ULL - 11644473600ULL;
}
#endif
