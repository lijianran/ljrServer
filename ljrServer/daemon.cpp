/**
 * @file daemon.cpp
 * @author lijianran (lijianran@outlook.com)
 * @brief 守护进程
 * @version 0.1
 * @date 2022-02-12
 */

#include "daemon.h"

// 日志
#include "log.h"
// 配置
#include "config.h"

// time
#include <time.h>
// strerror
#include <string.h>
// waitpid
#include <sys/types.h>
#include <sys/wait.h>

namespace ljrserver {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

// 配置 重启等待时间
static ljrserver::ConfigVar<uint32_t>::ptr g_daemon_restart_interval =
    ljrserver::Config::Lookup("daemon.restart_interval", (uint32_t)5,
                              "daemon restart interval");

/**
 * @brief 进程信息转字符串
 *
 * @return std::string
 */
std::string ProcessInfo::toString() const {
    std::stringstream ss;
    ss << "[ProcessInfo parent_id=" << parent_id << " main_id=" << main_id
       << " parent_start_time=" << ljrserver::Time2Str(parent_start_time)
       << " main_start_time=" << ljrserver::Time2Str(main_start_time)
       << " restart_count=" << restart_count << "]";
    return ss.str();
}

/**
 * @brief 主函数
 *
 * @param argc
 * @param argv
 * @param main_cb
 * @return int
 */
static int real_start(int argc, char** argv,
                      std::function<int(int argc, char** argv)> main_cb) {
    // 获取父进程 id
    ProcessInfoMgr::GetInstance()->parent_id = getpid();
    // 设置父进程启动时间
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);

    return main_cb(argc, argv);
}

/**
 * @brief 守护进程
 *
 * @param argc
 * @param argv
 * @param main_cb
 * @return int
 */
static int real_daemon(int argc, char** argv,
                       std::function<int(int argc, char** argv)> main_cb) {
    // 参数一将上下文路径指向根路径 参数二将标准输入输出流关闭
    daemon(1, 0);

    // 获取父进程 id
    ProcessInfoMgr::GetInstance()->parent_id = getpid();
    // 设置父进程启动时间
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);

    // 循环守护
    while (true) {
        // fork 子进程
        pid_t pid = fork();

        if (pid == 0) {
            // 获取子进程 id
            ProcessInfoMgr::GetInstance()->main_id = getpid();
            // 设置子进程启动时间
            ProcessInfoMgr::GetInstance()->main_start_time = time(0);
            // 日志
            LJRSERVER_LOG_INFO(g_logger) << "process start pid=" << getpid();
            // 执行主函数
            return real_start(argc, argv, main_cb);

        } else if (pid < 0) {
            // 有错误
            LJRSERVER_LOG_ERROR(g_logger)
                << "fork fail return=" << pid << " errno=" << errno
                << " errstr=" << strerror(errno);
            return -1;

        } else {
            // 父进程等待子进程
            int status = 0;
            waitpid(pid, &status, 0);

            // 子进程结束
            if (status) {
                // 异常退出
                LJRSERVER_LOG_ERROR(g_logger)
                    << "child crash pid=" << pid << " status=" << status;
            } else {
                // 正常退出
                LJRSERVER_LOG_INFO(g_logger) << "child finished pid=" << pid;
                // 退出守护进程
                break;
            }

            // 重启次数
            ProcessInfoMgr::GetInstance()->restart_count += 1;

            // 等待释放资源
            sleep(g_daemon_restart_interval->getValue());
        }
        // 重启子进程
    }

    return 0;
}

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
                 bool is_daemon) {
    if (!is_daemon) {
        // 执行主函数
        return real_start(argc, argv, main_cb);
    }
    // 执行守护函数
    return real_daemon(argc, argv, main_cb);
}

};  // namespace ljrserver