
#include "../ljrServer/tcp_server.h"
// #include "../ljrServer/iomanager.h"
#include "../ljrServer/log.h"

// 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 测试 tcp 服务器
 *
 */
void run() {
    // 监听地址
    auto addr = ljrserver::Address::LookupAny("0.0.0.0:8033");
    // auto addr2 = ljrserver::UnixAddress::ptr(
    //     new ljrserver::UnixAddress("/tmp/unix_addr"));

    // LJRSERVER_LOG_INFO(g_logger) << *addr << " - " << *addr2;

    LJRSERVER_LOG_INFO(g_logger) << *addr;

    std::vector<ljrserver::Address::ptr> addrs;
    std::vector<ljrserver::Address::ptr> fails;
    addrs.push_back(addr);
    // addrs.push_back(addr2);

    // tcp 服务器
    ljrserver::TcpServer::ptr tcp_server(new ljrserver::TcpServer);

    // 循环绑定地址
    while (!tcp_server->bind(addrs, fails)) {
        // 失败则等待 2s
        sleep(2);
    }

    // 开启 tcp 服务器
    tcp_server->start();
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    // IO 管理器
    ljrserver::IOManager iom(2);
    // 调度任务
    iom.schedule(run);
    return 0;
}
