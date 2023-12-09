#ifndef RLOG_H
#define RLOG_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RLOG_MAX_LOG_BUF_SIZE (512)
#define RLOG_MAX_ONE_LOG_SIZE (128)

// hook every printf to rlog_printf
#define RLOG_HOOK_PLAT_PRINTF

#ifdef RLOG_HOOK_PLAT_PRINTF
#define printf(format, ...)                 \
    {                                       \
        rlog_printf(format, ##__VA_ARGS__); \
        printf(format, ##__VA_ARGS__);      \
    }
#endif

#ifndef RLOG_TAG
#define RLOG_TAG ""
#endif

#ifndef LOG_TIMESTAMP
#define LOG_TIMESTAMP 0
#endif

typedef int hal_printf_api_t(const char *fmt, ...);
int         rlog_printf2buf(const char *fmt, ...);
void        rlog_dump(void *data, size_t count, size_t offset);
void        rlog_init(void);
int         rlog_size(void);

void rlog_dumphex(void);

#define LOG_LEVEL_FATAL  0
#define LOG_LEVEL_ERROR  10
#define LOG_LEVEL_WARN   20
#define LOG_LEVEL_INFO   30
#define LOG_LEVEL_DEBUG  40
#define LOG_LEVEL_PACKET 50
#define LOG_LEVEL_DUMP   60
#define LOG_LEVEL_TRACE  70

#define LOG_LEVEL_ALL 127

#ifndef LOG_LEVEL_DEFAULT
#define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO
#endif

#define LOG_SERVICE_GLOBAL    0x00000001
#define LOG_SERVICE_ETH_IN    0x00000002
#define LOG_SERVICE_ETH_OUT   0x00000004
#define LOG_SERVICE_RADIO_IN  0x00000008
#define LOG_SERVICE_RADIO_OUT 0x00000010
#define LOG_SERVICE_TAP_IN    0x00000020
#define LOG_SERVICE_TAP_OUT   0x00000040
#define LOG_SERVICE_SLIP_IN   0x00000080
#define LOG_SERVICE_SLIP_OUT  0x00000100
#define LOG_SERVICE_PF_IN     0x00000200
#define LOG_SERVICE_PF_OUT    0x00000400
#define LOG_SERVICE_SLIP_DBG  0x00000800

#define LOG_SERVICE_ALL 0xFFFFFFFF

#ifndef LOG_SERVICE_DEFAULT
#define LOG_SERVICE_DEFAULT LOG_SERVICE_ALL
#endif

#if LOG_TIMESTAMP
extern void log_timestamp(hal_printf_api_t hal_printf_api);
#define _LOG_ADD_TIMESTAMP(func) log_timestamp(func);
#else
#define _LOG_ADD_TIMESTAMP(func)
#endif

#define _LOG_LEVEL_F_REAL(level, func, ...) \
    {                                       \
        _LOG_ADD_TIMESTAMP(func)            \
        func(#level ":" RLOG_TAG ": ");     \
        func(__VA_ARGS__);                  \
    }

#define _LOG_LEVEL_A_REAL(func, ...) \
    {                                \
        func(__VA_ARGS__);           \
    }

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEFAULT
#endif

#ifndef LOG_SERVICE
#define LOG_SERVICE LOG_SERVICE_DEFAULT
#endif

#if LOG_LEVEL_FATAL <= LOG_LEVEL
#define _LEVEL_FILTER_FATAL(a) a
#else
#define _LEVEL_FILTER_FATAL(...)
#endif

#if LOG_LEVEL_ERROR <= LOG_LEVEL
#define _LEVEL_FILTER_ERROR(a) a
#else
#define _LEVEL_FILTER_ERROR(...)
#endif

#if LOG_LEVEL_WARN <= LOG_LEVEL
#define _LEVEL_FILTER_WARN(a) a
#else
#define _LEVEL_FILTER_WARN(...)
#endif

#if LOG_LEVEL_INFO <= LOG_LEVEL
#define _LEVEL_FILTER_INFO(a) a
#else
#define _LEVEL_FILTER_INFO(...)
#endif

#if LOG_LEVEL_DEBUG <= LOG_LEVEL
#define _LEVEL_FILTER_DEBUG(a) a
#else
#define _LEVEL_FILTER_DEBUG(...)
#endif

#if LOG_LEVEL_PACKET <= LOG_LEVEL
#define _LEVEL_FILTER_PACKET(a) a
#else
#define _LEVEL_FILTER_PACKET(...)
#endif

#if LOG_LEVEL_DUMP <= LOG_LEVEL
#define _LEVEL_FILTER_DUMP(a) a
#else
#define _LEVEL_FILTER_DUMP(...)
#endif

#if LOG_LEVEL_TRACE <= LOG_LEVEL
#define _LEVEL_FILTER_TRACE(a) a
#else
#define _LEVEL_FILTER_TRACE(...)
#endif

#if LOG_SERVICE & LOG_SERVICE_GLOBAL
#define _SERVICE_FILTER_GLOBAL(a) a
#else
#define _SERVICE_FILTER_GLOBAL(...)
#endif

#define _LOG_LEVEL_F(level, service, func, ...) \
    _SERVICE_FILTER_##service(                  \
        _LEVEL_FILTER_##level(_LOG_LEVEL_F_REAL(level, func, __VA_ARGS__)))
#define _LOG_LEVEL_A(level, service, func, ...) \
    _SERVICE_FILTER_##service(                  \
        _LEVEL_FILTER_##level(_LOG_LEVEL_A_REAL(func, __VA_ARGS__)))

#define LOG_COND_FUNC(level, service, func) \
    _SERVICE_FILTER_##service(_LEVEL_FILTER_##level(func))

#define LOG_PRINTF(level, service, ...) \
    _LOG_LEVEL_F(level, service, printf, __VA_ARGS__)
#define LOG_PRINTF2BUF(level, service, ...) \
    _LOG_LEVEL_F(level, service, rlog_printf2buf, __VA_ARGS__)
#define LOG_APPEND(level, service, ...) \
    _LOG_LEVEL_A(level, service, printf, __VA_ARGS__)
#define LOG_WRITE(level, service, buffer, size) \
    _LOG_LEVEL_F(level, service, fwrite, buffer, size, 1, stdout)

#if 0
#define LOG_FATAL(...)  LOG_PRINTF(FATAL, GLOBAL, __VA_ARGS__)
#define LOG_ERROR(...)  LOG_PRINTF(ERROR, GLOBAL, __VA_ARGS__)
#define LOG_WARN(...)   LOG_PRINTF(WARN, GLOBAL, __VA_ARGS__)
#define LOG_INFO(...)   LOG_PRINTF(INFO, GLOBAL, __VA_ARGS__)
#define LOG_DEBUG(...)  LOG_PRINTF(DEBUG, GLOBAL, __VA_ARGS__)
#define LOG_TRACE(...)  LOG_PRINTF(TRACE, GLOBAL, __VA_ARGS__)
#define LOG_PACKET(...) LOG_PRINTF(PACKET, GLOBAL, __VA_ARGS__)
#define LOG_DUMP(...)   LOG_PRINTF(DUMP, GLOBAL, __VA_ARGS__)
#endif

#if 0
#define LOG_FATAL(...)  LOG_PRINTF2BUF(FATAL, GLOBAL, __VA_ARGS__)
#define LOG_ERROR(...)  LOG_PRINTF2BUF(ERROR, GLOBAL, __VA_ARGS__)
#define LOG_WARN(...)   LOG_PRINTF2BUF(WARN, GLOBAL, __VA_ARGS__)
#define LOG_INFO(...)   LOG_PRINTF2BUF(INFO, GLOBAL, __VA_ARGS__)
#define LOG_DEBUG(...)  LOG_PRINTF2BUF(DEBUG, GLOBAL, __VA_ARGS__)
#define LOG_TRACE(...)  LOG_PRINTF2BUF(TRACE, GLOBAL, __VA_ARGS__)
#define LOG_PACKET(...) LOG_PRINTF2BUF(PACKET, GLOBAL, __VA_ARGS__)
#define LOG_DUMP(...)   LOG_PRINTF2BUF(DUMP, GLOBAL, __VA_ARGS__)
#endif

#define _LOG_NOLEVEL_F_REAL(level, func, ...) \
    {                                         \
        _LOG_ADD_TIMESTAMP(func)              \
        func(RLOG_TAG);                       \
        func(__VA_ARGS__);                    \
    }

#define _LOG_NOLEVEL_F(level, service, func, ...) \
    _SERVICE_FILTER_##service(                    \
        _LEVEL_FILTER_##level(_LOG_NOLEVEL_F_REAL(level, func, __VA_ARGS__)))

#define RLOG_PRINTF2BUF(level, service, ...) \
    _LOG_NOLEVEL_F(level, service, rlog_printf2buf, __VA_ARGS__)

#define rlog_printf(...) RLOG_PRINTF2BUF(INFO, GLOBAL, __VA_ARGS__)

#ifndef PRIu32
#define PRId8 "d"
#define PRIi8 "i"
#define PRIo8 "o"
#define PRIu8 "u"
#define PRIx8 "x"
#define PRIX8 "X"

#define PRId16 "hd"
#define PRIi16 "hi"
#define PRIo16 "ho"
#define PRIu16 "hu"
#define PRIx16 "hx"
#define PRIX16 "hX"

#define PRId32 "ld"
#define PRIi32 "li"
#define PRIo32 "lo"
#define PRIu32 "lu"
#define PRIx32 "lx"
#define PRIX32 "lX"

#define PRId64 "lld"
#define PRIi64 "lli"
#define PRIo64 "llo"
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRIX64 "llX"
#endif  ///#ifndef PRIu32

#ifdef __cplusplus
}
#endif

#endif  ///#ifndef TINYLOG_H
