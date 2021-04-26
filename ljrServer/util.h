
#ifndef __LJRSERVER_UTIL_H__
#define __LJRSERVER_UTIL_H__

// pid_t    syscall
// #include <pthread.h>
#include <unistd.h>
// #include <sys/types.h>
#include <sys/syscall.h>
// #include <stdio.h>

// #include <memory> uint32_t
#include <stdint.h>

namespace ljrserver
{

    // 线程id
    pid_t GetThreadId();

    // 协程id
    uint32_t GetFiberId();

}

#endif