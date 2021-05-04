
#ifndef __LJRSERVER_HTTP_SERVER_H__
#define __LJRSERVER_HTTP_SERVER_H__

#include "../tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace ljrserver
{

    namespace http
    {

        class HttpServer : public TcpServer
        {
        public:
            typedef std::shared_ptr<HttpServer> ptr;

            HttpServer(bool keepalive = false,
                       ljrserver::IOManager *worker = ljrserver::IOManager::GetThis(),
                       ljrserver::IOManager *accept_worker = ljrserver::IOManager::GetThis());

            ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }
            void setServletDispath(ServletDispatch::ptr v) { m_dispatch = v; }

        protected:
            void handleClient(Socket::ptr client) override;

        private:
            bool m_isKeepalive;

            ServletDispatch::ptr m_dispatch;
        };

    } // namespace http

} // namespace ljrserver

#endif //__LJRSERVER_HTTP_SERVER_H__