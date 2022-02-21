/**
 * @file ab_http_server.cpp
 * @author lijianran (lijianran@outlook.com)
 * @brief ab 测试 http 服务器
 * @version 0.1
 * @date 2022-02-14
 */

#include "../ljrServer/http/http_server.h"
#include "../ljrServer/log.h"

// 日志
ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 服务器
 *
 */
void run() {
    // info 日志级别
    g_logger->setLevel(ljrserver::LogLevel::INFO);
    // 地址
    ljrserver::Address::ptr addr =
        ljrserver::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if (!addr) {
        LJRSERVER_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    // ljrserver::http::HttpServer::ptr http_server(
    //     new ljrserver::http::HttpServer(true, worker.get()));
    ljrserver::http::HttpServer::ptr http_server(
        new ljrserver::http::HttpServer(true));

    // 绑定监听
    while (!http_server->bind(addr)) {
        LJRSERVER_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    // 启动服务器
    http_server->start();
}

/**
 * @brief 压力测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char** argv) {
    ljrserver::IOManager iom(2);

    iom.schedule(run);

    return 0;
}
