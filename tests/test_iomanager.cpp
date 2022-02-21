
// #include "../ljrServer/ljrserver.h"
#include "../ljrServer/iomanager.h"
#include "../ljrServer/log.h"

// string
#include <string.h>
// #include <sys/types.h>
// socket
#include <sys/socket.h>
// fcntl
#include <fcntl.h>
// 网络
#include <arpa/inet.h>
// epoll
#include <sys/epoll.h>
// io
#include <iostream>

// 日志
ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

// 句柄
int sock = 0;

/**
 * @brief 协程函数
 *
 */
void test_fiber() {
    LJRSERVER_LOG_INFO(g_logger) << "test_fiber";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    // 非阻塞
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // ip 地址
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    // inet_pton(AF_INET, "112.80.248.75", &addr.sin_addr.s_addr);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    // 连接
    if (!connect(sock, (const sockaddr *)&addr, sizeof(addr))) {
        // 连接失败

    } else if (errno == EINPROGRESS) {
        LJRSERVER_LOG_INFO(g_logger)
            << "add event errno=" << errno << " " << strerror(errno);

        // 读事件处理
        ljrserver::IOManager::GetThis()->addEvent(
            sock, ljrserver::IOManager::READ, []() {
                // 读事件
                LJRSERVER_LOG_INFO(g_logger) << "read callback";
            });

        // 写事件处理
        ljrserver::IOManager::GetThis()->addEvent(
            sock, ljrserver::IOManager::WRITE, []() {
                LJRSERVER_LOG_INFO(g_logger) << "write callback";
                // close(sock);

                ljrserver::IOManager::GetThis()->cancelEvent(
                    sock, ljrserver::IOManager::READ);

                // 关闭句柄
                close(sock);
            });

    } else {
        // 其他错误
        LJRSERVER_LOG_INFO(g_logger)
            << "else " << errno << " " << strerror(errno);
    }
}

/**
 * @brief 测试 iomanager
 *
 */
void test_iomanager() {
    // 读事件 (EPOLLIN)  READ  = 0x1,
    // 写事件 (EPOLLOUT) WRITE = 0x4,
    std::cout << "EPOLLIN = " << EPOLLIN << " EPOLLOUT = " << EPOLLOUT
              << std::endl;

    // caller 线程
    ljrserver::Thread::SetName("caller");

    // io manager
    ljrserver::IOManager iom(1, false, "iom");

    // 调度
    iom.schedule(&test_fiber);
}

// 定时器
ljrserver::Timer::ptr s_timer;

/**
 * @brief 测试定时器
 *
 */
void test_timer() {
    // caller 线程
    ljrserver::Thread::SetName("caller");

    // io manager
    ljrserver::IOManager iom(2, false, "test");

    // iom.addTimer(500, [](){
    //     LJRSERVER_LOG_INFO(g_logger) << "hello timer!";
    // }, true);

    // 添加定时器 1s 循环
    s_timer = iom.addTimer(
        1000,
        []() {
            static int i = 0;
            LJRSERVER_LOG_INFO(g_logger) << "hello timer! i = " << i;
            if (++i == 3) {
                // 重设 2s 执行
                s_timer->reset(2000, true);

                // 取消
                // s_timer->cancel();
            }
        },
        true);
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    // 测试 iomanager
    test_iomanager();

    // 测试定时器
    // test_timer();

    return 0;
}
