
#include "../ljrServer/tcp_server.h"
// #include "../ljrServer/iomanager.h"
#include "../ljrServer/log.h"

static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

void run() {
    auto addr = ljrserver::Address::LookupAny("0.0.0.0:8033");
    // auto addr2 = ljrserver::UnixAddress::ptr(new
    // ljrserver::UnixAddress("/tmp/unix_addr"));

    // LJRSERVER_LOG_INFO(g_logger) << *addr << " - " << *addr2;
    std::vector<ljrserver::Address::ptr> addrs;
    std::vector<ljrserver::Address::ptr> fails;

    addrs.push_back(addr);
    // addrs.push_back(addr2);

    ljrserver::TcpServer::ptr tcp_server(new ljrserver::TcpServer);
    while (!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
}

int main(int argc, char const *argv[]) {
    ljrserver::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
