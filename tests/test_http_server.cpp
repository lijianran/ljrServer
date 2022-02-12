
#include "../ljrServer/http/http_server.h"
#include "../ljrServer/log.h"

// 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 测试 http server 服务器以及 servlet
 *
 */
void run() {
    // 服务器实例
    ljrserver::http::HttpServer::ptr server(new ljrserver::http::HttpServer);

    // 监听地址
    ljrserver::Address::ptr addr =
        ljrserver::Address::LookupAnyIPAddress("0.0.0.0:12346");

    // 绑定监听地址
    while (!server->bind(addr)) {
        // bind 失败等待 2s 已经 hook 以定时器实现
        sleep(2);
    }

    // 获取 Servlet 派遣器
    auto sd = server->getServletDispatch();

    // 增加精准命中的 Servlet
    sd->addServlet("/lijianran/xx",
                   [](ljrserver::http::HttpRequest::ptr req,
                      ljrserver::http::HttpResponse::ptr rsp,
                      ljrserver::http::HttpSession::ptr session) {
                       rsp->setBody(req->toString());
                       return 0;
                   });

    // 增加模糊命中的 Servlet
    sd->addGlobServlet("/lijianran/*",
                       [](ljrserver::http::HttpRequest::ptr req,
                          ljrserver::http::HttpResponse::ptr rsp,
                          ljrserver::http::HttpSession::ptr session) {
                           rsp->setBody("Glob:\r\n" + req->toString());
                           return 0;
                       });

    sd->addGlobServlet("/liyaopeng/*",
                       [](ljrserver::http::HttpRequest::ptr req,
                          ljrserver::http::HttpResponse::ptr rsp,
                          ljrserver::http::HttpSession::ptr session) {
                           rsp->setBody("<h1>welcome, liyaopeng!</h1>");
                           return 0;
                       });

    sd->addGlobServlet("/lijing/*",
                       [](ljrserver::http::HttpRequest::ptr req,
                          ljrserver::http::HttpResponse::ptr rsp,
                          ljrserver::http::HttpSession::ptr session) {
                           rsp->setBody("<h1>welcome, lijing!</h1>");
                           return 0;
                       });

    sd->addGlobServlet("/jianhong/*",
                       [](ljrserver::http::HttpRequest::ptr req,
                          ljrserver::http::HttpResponse::ptr rsp,
                          ljrserver::http::HttpSession::ptr session) {
                           rsp->setBody("<h1>welcome, jianhong!</h1>");
                           return 0;
                       });

    sd->addGlobServlet("/liqian/*",
                       [](ljrserver::http::HttpRequest::ptr req,
                          ljrserver::http::HttpResponse::ptr rsp,
                          ljrserver::http::HttpSession::ptr session) {
                           rsp->setBody("<h1>welcome, liqian!</h1>");
                           return 0;
                       });

    // 启动服务器
    server->start();
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
