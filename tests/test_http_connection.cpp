
#include "../ljrServer/http/http_connection.h"
#include "../ljrServer/log.h"
#include "../ljrServer/iomanager.h"

#include <iostream>

// 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 测试连接池
 *
 */
void test_pool() {
    ljrserver::http::HttpConnectionPool::ptr pool(
        new ljrserver::http::HttpConnectionPool("www.baidu.com", "", 80, 10,
                                                1000 * 30, 5));

    // 添加循环定时器
    ljrserver::IOManager::GetThis()->addTimer(
        1000,
        [pool]() {
            // 每隔 1s 请求百度 返回 HttpResult
            auto r = pool->doGet("/", 300);
            // 打印结果
            LJRSERVER_LOG_INFO(g_logger) << r->toString();
        },
        true);
}

/**
 * @brief 测试客户端连接 http 服务器
 *
 */
void run() {
    // 服务器地址
    ljrserver::Address::ptr addr =
        ljrserver::Address::LookupAnyIPAddress("www.sylar.top:80");
    if (!addr) {
        // 地址转换失败
        LJRSERVER_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    // 根据地址协议簇创建 tcp 连接
    ljrserver::Socket::ptr sock = ljrserver::Socket::CreateTCP(addr);

    // 连接服务器
    bool rt = sock->connect(addr);
    if (!rt) {
        // 连接失败
        LJRSERVER_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    // 客户端
    ljrserver::http::HttpConnection::ptr conn(
        new ljrserver::http::HttpConnection(sock));

    // 请求
    ljrserver::http::HttpRequest::ptr req(new ljrserver::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");
    LJRSERVER_LOG_INFO(g_logger) << "req: " << std::endl << *req;

    // 发送请求
    conn->sendRequest(req);
    // 接收请求
    auto rsp = conn->recvResponse();
    if (!rsp) {
        // 接收失败
        LJRSERVER_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    // 打印响应内容
    LJRSERVER_LOG_INFO(g_logger) << "rsp: " << std::endl << *rsp;

    LJRSERVER_LOG_INFO(g_logger) << "================================";
    // 测试 HttpResult 响应结果
    ljrserver::http::HttpResult::ptr result =
        ljrserver::http::HttpConnection::DoGet("http://www.baidu.com", 300);
    // LJRSERVER_LOG_INFO(g_logger)
    //     << "result=" << result->result << " error=" << result->error
    //     << " rsp: " << std::endl
    //     << (result->response ? result->response->toString() : "");
    LJRSERVER_LOG_INFO(g_logger) << result->toString();
    LJRSERVER_LOG_INFO(g_logger) << "================================";

    // 测试连接池
    // test_pool();
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    // IO 调度器
    ljrserver::IOManager iom(2);
    // 调度任务
    iom.schedule(run);
    return 0;
}
