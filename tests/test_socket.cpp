
#include "../ljrServer/socket.h"
#include "../ljrServer/log.h"
#include "../ljrServer/iomanager.h"
// #include "../ljrServer/address.h"

static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

void test_socket()
{
    ljrserver::IPAddress::ptr addr = ljrserver::Address::LookupAnyIPAdress("www.baidu.com");
    if (addr)
    {
        LJRSERVER_LOG_INFO(g_logger) << "get address: " << addr->toString();
    }
    else
    {
        LJRSERVER_LOG_ERROR(g_logger) << "get address fail";
        return;
    }

    ljrserver::Socket::ptr sock = ljrserver::Socket::CreateTCP(addr);
    addr->setPort(80);
    if (!sock->connect(addr))
    {
        LJRSERVER_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    }
    else
    {
        LJRSERVER_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if (rt <= 0)
    {
        LJRSERVER_LOG_INFO(g_logger) << "send fail rt = " << rt;
        return;
    }

    std::string buffers;
    buffers.resize(4096);
    rt = sock->recv(&buffers[0], buffers.size());

    if (rt <= 0)
    {
        LJRSERVER_LOG_INFO(g_logger) << "recv fail rt = " << rt;
        return;
    }

    buffers.resize(rt);
    LJRSERVER_LOG_INFO(g_logger) << buffers;
}

int main(int argc, const char *argv[])
{

    ljrserver::IOManager iom;
    iom.schedule(&test_socket);

    return 0;
}
