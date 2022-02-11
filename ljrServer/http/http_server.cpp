
#include "http_server.h"
#include "../log.h"

namespace ljrserver {

namespace http {

static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive, ljrserver::IOManager *worker,
                       ljrserver::IOManager *accept_worker)
    : TcpServer(worker, accept_worker), m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr client) {
    ljrserver::http::HttpSession::ptr session(new HttpSession(client));

    do {
        auto req = session->recvRequest();
        if (!req) {
            LJRSERVER_LOG_WARN(g_logger)
                << "recv http request fail, errno = " << errno
                << " errno-string = " << strerror(errno)
                << " client:" << *client;
            break;
        }

        HttpResponse::ptr rsp(new HttpResponse(
            req->getVersion(), req->isCose() || !m_isKeepalive));

        m_dispatch->handle(req, rsp, session);
        // rsp->setBody("hello lijianran");

        // LJRSERVER_LOG_INFO(g_logger) << "request: " << std::endl
        //                              << *req;
        // LJRSERVER_LOG_INFO(g_logger) << "response: " << std::endl
        //                              << *rsp;

        session->sendResponse(rsp);

    } while (m_isKeepalive);

    session->close();
}

}  // namespace http

}  // namespace ljrserver
