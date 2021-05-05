
#include "../ljrServer/http/http_connection.h"
#include "../ljrServer/log.h"
#include "../ljrServer/iomanager.h"

#include <iostream>

static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

void test_pool()
{
    ljrserver::http::HttpConnectionPool::ptr pool(new ljrserver::http::HttpConnectionPool(
        "www.baidu.com", "", 80, 10, 1000 * 30, 5));

    ljrserver::IOManager::GetThis()->addTimer(
        1000, [pool]() {
            auto r = pool->doGet("/", 300);
            LJRSERVER_LOG_INFO(g_logger) << r->toString();
        },
        true);
}

void run()
{
    ljrserver::Address::ptr addr = ljrserver::Address::LookupAnyIPAddress("www.sylar.top:80");
    if (!addr)
    {
        LJRSERVER_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    ljrserver::Socket::ptr sock = ljrserver::Socket::CreateTCP(addr);

    bool rt = sock->connect(addr);
    if (!rt)
    {
        LJRSERVER_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    ljrserver::http::HttpConnection::ptr conn(new ljrserver::http::HttpConnection(sock));

    ljrserver::http::HttpRequest::ptr req(new ljrserver::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");
    LJRSERVER_LOG_INFO(g_logger) << "req: " << std::endl
                                 << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if (!rsp)
    {
        LJRSERVER_LOG_INFO(g_logger) << "recv response error";
        return;
    }

    LJRSERVER_LOG_INFO(g_logger) << "rsp: " << std::endl
                                 << *rsp;

    LJRSERVER_LOG_INFO(g_logger) << "=============================================";

    ljrserver::http::HttpResult::ptr result = ljrserver::http::HttpConnection::DoGet("http://www.baidu.com", 300);
    LJRSERVER_LOG_INFO(g_logger) << "result = " << result->result
                                 << " error = " << result->error
                                 << " rsp = " << (result->response ? result->response->toString() : "");

    LJRSERVER_LOG_INFO(g_logger) << "=============================================";

    test_pool();

}

int main(int argc, char const *argv[])
{
    ljrserver::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
