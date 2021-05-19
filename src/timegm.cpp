#include "timegm.h"
#include <time.h>
#include <stdlib.h>

#ifdef _WIN32
#include <errno.h>
#include <timezoneapi.h>

time_t timegm2(struct tm *tm) {
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

#else

// We use this function instead of timegm(), because timegm() is a nonstandard
// GNU extension.
time_t timegm2(struct tm *tm) {
  time_t ret;
  char *tz;

  tz = getenv("TZ");
  setenv("TZ", "", 1);
  tzset();
  ret = mktime(tm);
  if (tz)
      setenv("TZ", tz, 1);
  else
      unsetenv("TZ");
  tzset();
  return ret;
}

#endif
