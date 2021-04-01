#ifndef TIMEGM_H
#define TIMEGM_H

#include <time.h>

#ifdef _WIN32
time_t timegm(struct tm *tm);
#endif

#endif
