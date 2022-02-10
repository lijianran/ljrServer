
#include "../ljrServer/socket.h"
#include "../ljrServer/log.h"
#include "../ljrServer/iomanager.h"
// #include "../ljrServer/address.h"

// 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 测试 socket
 *
 */
void test_socket() {
    // 地址
    ljrserver::IPAddress::ptr addr =
        ljrserver::Address::LookupAnyIPAddress("www.baidu.com");
    if (addr) {
        LJRSERVER_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        LJRSERVER_LOG_ERROR(g_logger) << "get address fail";
        return;
    }

    // 根据地址的协议簇 创建 socket 对象
    ljrserver::Socket::ptr sock = ljrserver::Socket::CreateTCP(addr);
    // 设置端口号 小端
    addr->setPort(80);

    // 连接地址
    if (!sock->connect(addr)) {
        // 连接失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "connect " << addr->toString() << " fail";
        return;
    } else {
        // 连接成功
        LJRSERVER_LOG_INFO(g_logger)
            << "connect " << addr->toString() << " connected";
    }

    // 要向 socket 发送的内容
    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    LJRSERVER_LOG_INFO(g_logger) << "发送消息:" << std::endl << buff;

    // 发送消息
    int rt = sock->send(buff, sizeof(buff));
    if (rt <= 0) {
        // 发送失败
        LJRSERVER_LOG_INFO(g_logger) << "send fail rt = " << rt;
        return;
    }

    // 响应消息
    std::string buffers;
    buffers.resize(4096);

    // 接收响应消息
    rt = sock->recv(&buffers[0], buffers.size());
    if (rt <= 0) {
        // 接收失败
        LJRSERVER_LOG_INFO(g_logger) << "recv fail rt = " << rt;
        return;
    }

    // 输出响应消息
    buffers.resize(rt);
    LJRSERVER_LOG_INFO(g_logger) << "响应消息:" << std::endl << buffers;
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, const char *argv[]) {
    // caller 线程名称
    ljrserver::Thread::SetName("caller");
    // IO 管理器
    ljrserver::IOManager iom;
    // 调度
    iom.schedule(&test_socket);

    return 0;
}
