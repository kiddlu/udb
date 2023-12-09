#include <inttypes.h>
#include <stdio.h>

#include "utils.h"

void dumphex(void *data, uint32_t size)
{
    char         ascii[17];
    unsigned int i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i)
    {
        printf("%02X ", ((unsigned char *)data)[i]);
        if (((unsigned char *)data)[i] >= ' ' &&
            ((unsigned char *)data)[i] <= '~')
        {
            ascii[i % 16] = ((unsigned char *)data)[i];
        }
        else
        {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size)
        {
            printf(" ");
            if ((i + 1) % 16 == 0)
            {
                printf("|  %s \n", ascii);
            }
            else if (i + 1 == size)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j)
                {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}

#ifdef _WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t    clock;
    struct tm tm;

    SYSTEMTIME wtm;
    GetLocalTime(&wtm);

    tm.tm_year  = wtm.wYear - 1900;
    tm.tm_mon   = wtm.wMonth - 1;
    tm.tm_mday  = wtm.wDay;
    tm.tm_hour  = wtm.wHour;
    tm.tm_min   = wtm.wMinute;
    tm.tm_sec   = wtm.wSecond;
    tm.tm_isdst = -1;
    clock       = mktime(&tm);

    tp->tv_sec  = (long)clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return (0);
}
#endif

unsigned long get_time(void)
{
    struct timeval ts;
    gettimeofday(&ts, NULL);
    return (ts.tv_sec * 1000 + ts.tv_usec / 1000);
}
