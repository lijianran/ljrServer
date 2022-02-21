
#include "http_server.h"
// 日志
#include "../log.h"

namespace ljrserver {

namespace http {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/**
 * @brief http 服务器构造函数
 *
 * @param keepalive 长连接 [= false]
 * @param worker 工作协程 [= ljrserver::IOManager::GetThis()]
 * @param accept_worker 等待接受客户端连接的协程 [=
 * ljrserver::IOManager::GetThis()]
 */
HttpServer::HttpServer(bool keepalive, ljrserver::IOManager *worker,
                       ljrserver::IOManager *accept_worker)
    : TcpServer(worker, accept_worker), m_isKeepalive(keepalive) {
    // 初始化构造 TcpServer
    // 初始化 Servlet
    m_dispatch.reset(new ServletDispatch);
}

/**
 * @brief 基类 tcp 服务器虚函数的实现 处理客户端的连接
 *
 * @param client 客户端 socket 对象
 */
void HttpServer::handleClient(Socket::ptr client) {
    // 创建连接 session
    ljrserver::http::HttpSession::ptr session(new HttpSession(client));

    do {
        // 接收客户端 request 请求
        auto req = session->recvRequest();
        if (!req) {
            // 接收失败
            // LJRSERVER_LOG_WARN(g_logger)
            //     << "recv http request fail, errno=" << errno
            //     << " errno-string=" << strerror(errno) << " client:" <<
            //     *client;
            LJRSERVER_LOG_DEBUG(g_logger)
                << "recv http request fail, errno=" << errno
                << " errno-string=" << strerror(errno) << " client:" << *client;
            break;
        }

        // 构建响应对象
        HttpResponse::ptr rsp(new HttpResponse(
            req->getVersion(), req->isCose() || !m_isKeepalive));

        // rsp->setBody("hello lijianran");

        // LJRSERVER_LOG_INFO(g_logger) << "request: " << std::endl << *req;
        // LJRSERVER_LOG_INFO(g_logger) << "response: " << std::endl << *rsp;

        // Servlet
        m_dispatch->handle(req, rsp, session);

        // 服务端通过 socket stream 发送响应
        session->sendResponse(rsp);

        if (!m_isKeepalive || req->isCose()) {
            break;
        }

    } while (m_isKeepalive);
    // 长连接

    // 关闭 session 服务端
    session->close();
}

}  // namespace http

}  // namespace ljrserver
