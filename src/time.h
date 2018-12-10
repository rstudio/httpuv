// Convert struct tm to seconds since Unix epoch, in GMT. Would use timegm()
// instead, but I don't think it's available on Solaris.
//
// From https://stackoverflow.com/a/11324281/412655
inline time_t my_timegm(const struct tm* t) {
  long year;
  time_t result;
  #define MONTHSPERYEAR   12      /* months per calendar year */
  static const int cumdays[MONTHSPERYEAR] =
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

  /*@ +matchanyintegral @*/
  year = 1900 + t->tm_year + t->tm_mon / MONTHSPERYEAR;
  result = (year - 1970) * 365 + cumdays[t->tm_mon % MONTHSPERYEAR];
  result += (year - 1968) / 4;
  result -= (year - 1900) / 100;
  result += (year - 1600) / 400;
  if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0) &&
    (t->tm_mon % MONTHSPERYEAR) < 2)
    result--;
  result += t->tm_mday - 1;
  result *= 24;
  result += t->tm_hour;
  result *= 60;
  result += t->tm_min;
  result *= 60;
  result += t->tm_sec;
  if (t->tm_isdst == 1)
      result -= 3600;
  /*@ -matchanyintegral @*/
  return (result);
}
