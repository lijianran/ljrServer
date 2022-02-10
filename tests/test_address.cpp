
#include "../ljrServer/address.h"
#include "../ljrServer/log.h"

ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

void test() {
    std::vector<ljrserver::Address::ptr> addrs;

    // bool v = ljrserver::Address::Lookup(addrs, "www.baidu.com");
    // bool v = ljrserver::Address::Lookup(addrs, "www.baidu.com:80");
    // bool v = ljrserver::Address::Lookup(addrs, "www.baidu.com:http");
    bool v = ljrserver::Address::Lookup(addrs, "www.baidu.com:ftp");

    if (!v) {
        LJRSERVER_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for (size_t i = 0; i < addrs.size(); ++i) {
        LJRSERVER_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<ljrserver::Address::ptr, uint32_t>>
        results;
    bool v = ljrserver::Address::GetInterfaceAddresses(results);

    if (!v) {
        LJRSERVER_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for (auto &i : results) {
        LJRSERVER_LOG_INFO(g_logger)
            << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    auto addr = ljrserver::IPAddress::Create("www.baidu.com");

    if (addr) {
        LJRSERVER_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char const *argv[]) {
    // test();

    // test_iface();

    test_ipv4();

    return 0;
}
