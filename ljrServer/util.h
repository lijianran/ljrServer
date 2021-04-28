
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

#include <vector>
#include <string>

namespace ljrserver
{

    // 线程id
    pid_t GetThreadId();

    // 协程id
    uint32_t GetFiberId();

    // 函数调用栈
    void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

    // 函数调用栈，string
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");

}

#endif