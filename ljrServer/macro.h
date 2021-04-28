
#ifndef __LJRSERVER_MACRO_H__
#define __LJRSERVER_MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

#define LJRSERVER_ASSERT(x)                                                                         \
    if (!(x))                                                                                       \
    {                                                                                               \
        LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT()) << "ASSERTION: " #x                               \
                                                  << "\nbacktrace:\n"                               \
                                                  << ljrserver::BacktraceToString(100, 2, "     "); \
        assert(x);                                                                                  \
    }

#define LJRSERVER_ASSERT2(x, w)                                                                     \
    if (!(x))                                                                                       \
    {                                                                                               \
        LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT()) << "ASSERTION: " #x                               \
                                                  << "\n"                                           \
                                                  << w                                              \
                                                  << "\nbacktrace:\n"                               \
                                                  << ljrserver::BacktraceToString(100, 2, "     "); \
        assert(x);                                                                                  \
    }

#endif