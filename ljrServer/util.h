
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
void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

/**
 * @brief 函数调用栈 string
 *
 * @param size
 * @param skip
 * @param prefix
 * @return std::string
 */
std::string BacktraceToString(int size = 64, int skip = 2,
                              const std::string& prefix = "");

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

/**
 * @brief 时间字符串
 *
 * @param ts
 * @return std::string
 */
std::string Time2Str(time_t ts = time(0),
                     const std::string& format = "%Y-%m-%d %H:%M:%s");

/**
 * @brief Class 文件相关工具类
 *
 */
class FSUtil {
public:
    /**
     * @brief 列出文件夹下所有文件
     *
     * @param files
     * @param path
     * @param subfix
     */
    static void ListAllFile(std::vector<std::string>& files,
                            const std::string& path, const std::string& subfix);

    /**
     * @brief mkdir
     *
     * @param dirname
     * @return true
     * @return false
     */
    static bool Mkdir(const std::string& dirname);

    /**
     * @brief 通过 pid file 来判断程序是否已经启动
     * 
     * @param pidfile 
     * @return true 
     * @return false 
     */
    static bool IsRunningPidfile(const std::string& pidfile);
};

}  // namespace ljrserver

#endif