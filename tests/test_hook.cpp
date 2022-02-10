
#include "../ljrServer/hook.h"
#include "../ljrServer/log.h"
#include "../ljrServer/iomanager.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 日志
ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 测试 hook sleep
 *
 */
void test_sleep() {
    // IO 调度器
    ljrserver::IOManager iom(1);

    // 调度任务 1
    iom.schedule([]() {
        sleep(2);  // hook
        // 2s 后输出
        LJRSERVER_LOG_INFO(g_logger) << "sleep 2";
    });

    // 调度任务 2
    iom.schedule([]() {
        sleep(3);  // hook
        // 1s 后输出
        LJRSERVER_LOG_INFO(g_logger) << "sleep 3";
    });

    // 非阻塞
    LJRSERVER_LOG_INFO(g_logger) << "sleep test";
}

/**
 * @brief 测试 socket
 *
 */
void test_sock() {
    // 创建 socket 句柄
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // 地址
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    // baidu
    inet_pton(AF_INET, "14.215.177.39", &addr.sin_addr.s_addr);

    LJRSERVER_LOG_INFO(g_logger) << "begin connect";

    // 客户端连接服务器地址
    int rt = connect(sock, (const sockaddr *)&addr, sizeof(addr));

    LJRSERVER_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if (rt) {
        // 连接失败
        return;
    }

    // 要向 socket 发送的内容
    // const char data[] = "GET / HTTP/1.0\r\n\r\n";
    const char data[] = "GET / HTTP/1.1\r\n\r\n";
    LJRSERVER_LOG_INFO(g_logger) << "发送消息:" << std::endl << data;

    // 向服务器发送消息
    rt = send(sock, data, sizeof(data), 0);

    LJRSERVER_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if (rt <= 0) {
        // 发送失败
        return;
    }

    // 响应消息
    std::string buff;
    buff.resize(4096);

    // 接收响应消息
    rt = recv(sock, &buff[0], buff.size(), 0);
    LJRSERVER_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if (rt <= 0) {
        // 接收失败
        return;
    }

    // 输出响应消息
    buff.resize(rt);
    LJRSERVER_LOG_INFO(g_logger) << "响应消息:" << std::endl << buff;
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    // 测试 hook sleep
    // test_sleep();

    // 测试 socket
    // test_sock();

    // 测试调度
    // IO 调度器
    ljrserver::IOManager iom;
    // 调度任务
    iom.schedule(test_sock);

    return 0;
}
