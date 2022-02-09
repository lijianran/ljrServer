
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

namespace ljrserver {

/**
 * @brief 获取当前线程 id
 *
 * @return pid_t
 */
pid_t GetThreadId();

/**
 * @brief 获取协程 id
 *
 * @return uint32_t
 */
uint32_t GetFiberId();

/**
 * @brief 函数调用栈
 *
 * @param bt
 * @param size
 * @param skip
 */
void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

/**
 * @brief 函数调用栈 string
 *
 * @param size
 * @param skip
 * @param prefix
 * @return std::string
 */
std::string BacktraceToString(int size = 64, int skip = 2,
                              const std::string &prefix = "");

/**
 * @brief 获取时间 ms
 *
 * @return uint64_t
 */
uint64_t GetCurrentMS();

/**
 * @brief 获取时间 us
 *
 * @return uint64_t
 */
uint64_t GetCurrentUS();

}  // namespace ljrserver

#endif