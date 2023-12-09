#ifndef _UTILS_H
#define _UTILS_H

#include "platform.h"

#include <inttypes.h>
void dumphex(void *data, uint32_t size);

#ifdef _WIN32
#include <time.h>
#include <windows.h>
int gettimeofday(struct timeval *tp, void *tzp);
#endif

unsigned long get_time(void);
#endif
