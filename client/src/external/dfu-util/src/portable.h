
#ifndef PORTABLE_H
#define PORTABLE_H
#include "config.h"

#ifndef HAVE_CONFIG_H
# define PACKAGE "dfu-util"
# define PACKAGE_VERSION "0.11-msvc"
# define PACKAGE_STRING "dfu-util 0.11-msvc"
# define PACKAGE_BUGREPORT "http://sourceforge.net/p/dfu-util/tickets/"
# include <io.h>
/* FIXME if off_t is a typedef it is not a define */
# ifndef off_t
#  define off_t long int
# endif
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_NANOSLEEP
# include <time.h>
# define milli_sleep(msec) do {\
  if (msec != 0) {\
    struct timespec nanosleepDelay = { (msec) / 1000, ((msec) % 1000) * 1000000 };\
    nanosleep(&nanosleepDelay, NULL);\
  } } while (0)
#elif defined HAVE_WINDOWS_H
# define milli_sleep(msec) do {\
  if (msec != 0) {\
    Sleep(msec);\
  } } while (0)
#else
# error "Can't get no sleep! Please report"
#endif /* HAVE_NANOSLEEP */

#ifdef HAVE_ERR
# include <err.h>
#else
# include <errno.h>
# include <string.h>
# define warnx(...) do {\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n"); } while (0)
# define errx(eval, ...) do {\
    warnx(__VA_ARGS__);\
    exit(eval); } while (0)
# define warn(...) do {\
    fprintf(stderr, "%s: ", strerror(errno));\
    warnx(__VA_ARGS__); } while (0)
# define err(eval, ...) do {\
    warn(__VA_ARGS__);\
    exit(eval); } while (0)
#endif /* HAVE_ERR */

#ifdef HAVE_SYSEXITS_H
# include <sysexits.h>
#else
# define EX_OK		0	/* successful termination */
# define EX_USAGE	64	/* command line usage error */
# define EX_DATAERR	65
# define EX_NOINPUT	66
# define EX_SOFTWARE	70	/* internal software error */
# define EX_CANTCREAT	73	/* input/output error */
# define EX_IOERR	74	/* input/output error */
# define EX_PROTOCOL	76	/* input/output error */
#endif /* HAVE_SYSEXITS_H */

#ifndef O_BINARY
# define O_BINARY   0
#endif

#endif /* PORTABLE_H */
