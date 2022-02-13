/**
 * @file daemon.h
 * @author lijianran (lijianran@outlook.com)
 * @brief 守护进程
 * @version 0.1
 * @date 2022-02-12
 */

#pragma once

// fork
#include <unistd.h>
// 函数包装
#include <functional>

// 单例模式
#include "singleton.h"

namespace ljrserver {

/**
 * @brief 守护进程信息
 *
 */
struct ProcessInfo {
    // 父进程 id
    pid_t parent_id = 0;

    // 主进程 id
    pid_t main_id = 0;

    // 父进程启动时间
    uint64_t parent_start_time = 0;

    // 主进程启动时间
    uint64_t main_start_time = 0;

    // 主进程重启次数
    uint32_t restart_count = 0;

    /**
     * @brief 进程信息转字符串
     * 
     * @return std::string 
     */
    std::string toString() const;
};

typedef ljrserver::Singleton<ProcessInfo> ProcessInfoMgr;

/**
 * @brief 启动守护进程
 *
 * @param argc
 * @param argv
 * @param main_cb
 * @param is_daemon
 * @return int
 */
int start_daemon(int argc, char** argv,
                 std::function<int(int argc, char** argv)> main_cb,
                 bool is_daemon);

};  // namespace ljrserver
