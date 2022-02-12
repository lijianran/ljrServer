
#ifndef __LJRSERVER_HTTP_SERVER_H__
#define __LJRSERVER_HTTP_SERVER_H__

// tcp 服务器虚基类
#include "../tcp_server.h"
// 连接 session 服务端抽象实现
#include "http_session.h"
// Servlet
#include "servlet.h"

namespace ljrserver {

namespace http {

/**
 * @brief Class 封装 http 服务器
 *
 * 继承自虚基类 TcpServer 实现的 http 服务器
 *
 */
class HttpServer : public TcpServer {
public:
    // 智能指针
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief http 服务器构造函数
     *
     * @param keepalive 长连接 [= false]
     * @param worker 工作协程 [= ljrserver::IOManager::GetThis()]
     * @param accept_worker 等待接受客户端连接的协程 [=
     * ljrserver::IOManager::GetThis()]
     */
    HttpServer(
        bool keepalive = false,
        ljrserver::IOManager *worker = ljrserver::IOManager::GetThis(),
        ljrserver::IOManager *accept_worker = ljrserver::IOManager::GetThis());

    /**
     * @brief 获取 Servlet
     *
     * @return ServletDispatch::ptr
     */
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }

    /**
     * @brief 设置 Servlet
     *
     * @param v
     */
    void setServletDispath(ServletDispatch::ptr v) { m_dispatch = v; }

protected:
    /**
     * @brief 基类 tcp 服务器虚函数的实现 处理客户端的连接
     *
     * @param client 客户端 socket 对象
     */
    void handleClient(Socket::ptr client) override;

private:
    // 是否长连接
    bool m_isKeepalive;

    // Servlet
    ServletDispatch::ptr m_dispatch;
};

}  // namespace http

}  // namespace ljrserver

#endif  //__LJRSERVER_HTTP_SERVER_H__