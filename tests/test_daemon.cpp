/**
 * @file test_daemon.cpp
 * @author lijianran (lijianran@outlook.com)
 * @brief 测试守护进程
 * @version 0.1
 * @date 2022-02-12
 */

#include "ljrServer/daemon.h"
#include "ljrServer/iomanager.h"
#include "ljrServer/log.h"

// 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 测试守护进程
 *
 * @param argc
 * @param argv
 * @return int
 */
int server_main(int argc, char** argv) {
    // 输出进程信息
    LJRSERVER_LOG_INFO(g_logger)
        << ljrserver::ProcessInfoMgr::GetInstance()->toString();

    // IO 调度器
    ljrserver::IOManager iom(1);

    // 循环计时器
    ljrserver::Timer::ptr timer = iom.addTimer(
        1000,
        [timer]() {
            LJRSERVER_LOG_INFO(g_logger) << "onTimer";

            static int count = 0;
            if (++count > 5) {
                // 正常退出
                exit(0);
                // 异常退出
                // exit(1);
            }
        },
        true);

    return 0;
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char** argv) {
    // 无参数则正常启动 有参数则守护进程
    return ljrserver::start_daemon(argc, argv, server_main, argc != 1);
}