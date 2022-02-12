
#ifndef __LJRSERVER_HTTP_SESSION_H__
#define __LJRSERVER_HTTP_SESSION_H__

#include "http.h"
#include "../socket_stream.h"

namespace ljrserver {

namespace http {

/**
 * @brief Class 封装 http session 会话
 *
 * 继承自 SocketStream 实现的 http 服务端
 *
 */
class HttpSession : public SocketStream {
public:
    // 智能指针
    typedef std::shared_ptr<HttpSession> ptr;

    /**
     * @brief 构造 http session 会话
     *
     * @param sock socket 句柄对象
     * @param owner 是否持有 [= true]
     */
    HttpSession(Socket::ptr sock, bool owner = true);

    /**
     * @brief 服务端接收请求
     *
     * @return HttpRequest::ptr
     */
    HttpRequest::ptr recvRequest();

    /**
     * @brief 服务端通过 socket stream 发送响应
     *
     * @param rsp http 响应对象
     * @return int
     */
    int sendResponse(HttpResponse::ptr rsp);
};

}  // namespace http

}  // namespace ljrserver

#endif  //__LJRSERVER_HTTP_SESSION_H__