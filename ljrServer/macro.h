
#ifndef __LJRSERVER_MACRO_H__
#define __LJRSERVER_MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

#if defined __GNUC__ || defined __llvm__
#define LJRSERVER_LICKLY(x) __builtin_expect(!!(x), 1)
#define LJRSERVER_UNLICKLY(x) __builtin_expect(!!(x), 0)
#else
#define LJRSERVER_LICKLY(x) (x)
#define LJRSERVER_UNLICKLY(x) (x)
#endif

#define LJRSERVER_ASSERT(x)                                                                         \
    if (LJRSERVER_UNLICKLY(!(x)))                                                                   \
    {                                                                                               \
        LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT()) << "ASSERTION: " #x                               \
                                                  << "\nbacktrace:\n"                               \
                                                  << ljrserver::BacktraceToString(100, 2, "     "); \
        assert(x);                                                                                  \
    }

#define LJRSERVER_ASSERT2(x, w)                                                                     \
    if (LJRSERVER_UNLICKLY(!(x)))                                                                   \
    {                                                                                               \
        LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT()) << "ASSERTION: " #x                               \
                                                  << "\n"                                           \
                                                  << w                                              \
                                                  << "\nbacktrace:\n"                               \
                                                  << ljrserver::BacktraceToString(100, 2, "     "); \
        assert(x);                                                                                  \
    }

#endif