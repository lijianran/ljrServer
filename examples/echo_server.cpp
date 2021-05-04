
#include "../ljrServer/tcp_server.h"
#include "../ljrServer/log.h"
// #include "../ljrServer/iomanager.h"
#include "../ljrServer/bytearray.h"

static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

class EchoServer : public ljrserver::TcpServer
{
public:
    typedef std::shared_ptr<EchoServer> ptr;

    EchoServer(int type);

    void handleClient(ljrserver::Socket::ptr client);

private:
    int m_type = 0;
};

EchoServer::EchoServer(int type) : m_type(type)
{
}

void EchoServer::handleClient(ljrserver::Socket::ptr client)
{
    LJRSERVER_LOG_INFO(g_logger) << "handleClient " << *client;
    ljrserver::ByteArray::ptr ba(new ljrserver::ByteArray);
    while (true)
    {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        if (rt == 0)
        {
            LJRSERVER_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        }
        else if (rt < 0)
        {
            LJRSERVER_LOG_ERROR(g_logger) << "client error rt = " << rt
                                          << " errno = " << errno
                                          << " errno-string = " << strerror(errno);
            break;
        }

        ba->setPostion(ba->getPosition() + rt);
        ba->setPostion(0);
        // LJRSERVER_LOG_INFO(g_logger) << "recv rt = " << rt << " data = " << std::string((char *)iovs[0].iov_base, rt);

        if (m_type == 1) // text
        {
            // LJRSERVER_LOG_INFO(g_logger) << ba->toString();
            std::cout << ba->toString();
        }
        else
        {
            // LJRSERVER_LOG_INFO(g_logger) << ba->toHexString();
            std::cout << ba->toHexString();
        }
    }
}

int type = 1;

void run()
{
    EchoServer::ptr es(new EchoServer(type));
    auto addr = ljrserver::Address::LookupAny("0.0.0.0:8023");
    while (!es->bind(addr))
    {
        sleep(2);
    }
    es->start();
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        LJRSERVER_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }

    if (!strcmp(argv[1], "-b"))
    {
        type = 2;
    }

    ljrserver::IOManager iom(2);

    iom.schedule(run);

    return 0;
}
