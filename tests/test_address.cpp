
#include "../ljrServer/address.h"
#include "../ljrServer/log.h"

ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 测试地址模块
 *
 */
void test_address() {
    // 地址列表
    std::vector<ljrserver::Address::ptr> addrs;

    // 获取地址列表
    // bool v = ljrserver::Address::Lookup(addrs, "www.baidu.com");
    // bool v = ljrserver::Address::Lookup(addrs, "www.baidu.com:80");
    // bool v = ljrserver::Address::Lookup(addrs, "www.baidu.com:http");
    bool v = ljrserver::Address::Lookup(addrs, "www.baidu.com:ftp");

    if (!v) {
        // 获取失败
        LJRSERVER_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    // 循环输出 addr:port
    for (size_t i = 0; i < addrs.size(); ++i) {
        LJRSERVER_LOG_INFO(g_logger) << i << ": " << addrs[i]->toString();
    }

    auto addr = ljrserver::Address::LookupAny("localhost:4080");
    if (addr) {
        LJRSERVER_LOG_INFO(g_logger) << *addr;
    } else {
        LJRSERVER_LOG_ERROR(g_logger) << "error";
    }
}

/**
 * @brief 测试网卡信息
 *
 */
void test_iface() {
    // 网卡信息
    std::multimap<std::string, std::pair<ljrserver::Address::ptr, uint32_t>>
        results;

    // 获取网卡信息
    bool v = ljrserver::Address::GetInterfaceAddresses(results);

    if (!v) {
        // 获取失败
        LJRSERVER_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    // 循环输出 name: addr - maskbits
    for (auto &i : results) {
        LJRSERVER_LOG_INFO(g_logger)
            << i.first << ": " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

/**
 * @brief 测试 IPv4 网络地址
 *
 */
void test_ipv4() {
    // IPAddress
    // auto addr = ljrserver::IPAddress::Create("www.baidu.com");
    auto addr = ljrserver::IPAddress::Create("127.0.0.8");

    if (addr) {
        // 输出地址
        LJRSERVER_LOG_INFO(g_logger) << addr->toString();
    }
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    // 测试地址模块
    LJRSERVER_LOG_INFO(g_logger) << "测试地址模块";
    test_address();

    // 测试网卡信息
    LJRSERVER_LOG_INFO(g_logger) << "测试网卡信息";
    test_iface();

    // 测试 IPv4 网络地址
    LJRSERVER_LOG_INFO(g_logger) << "测试 IPv4 网络地址";
    test_ipv4();

    return 0;
}
