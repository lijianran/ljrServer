
#ifndef __LJRSERVER_TCP_SERVER_H__
#define __LJRSERVER_TCP_SERVER_H__

#include <memory>
#include <functional>

#include "iomanager.h"
#include "socket.h"
#include "noncopyable.h"

namespace ljrserver {

/**
 * @brief tcp 服务器
 *
 * 虚基类
 * 继承自 std::enable_shared_from_this
 * 继承自 Noncopyable 不可复制
 *
 */
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    // 智能指针
    typedef std::shared_ptr<TcpServer> ptr;

    /**
     * @brief tcp 服务器的构造函数
     *
     * @param worker 工作协程 [= ljrserver::IOManager::GetThis()]
     * @param acceptworker 等待接受客户端连接的协程 [=
     * ljrserver::IOManager::GetThis()]
     */
    TcpServer(
        ljrserver::IOManager *worker = ljrserver::IOManager::GetThis(),
        ljrserver::IOManager *acceptworker = ljrserver::IOManager::GetThis());

    /**
     * @brief tcp 服务器的析构函数 基类析构函数设置为虚函数
     *
     */
    virtual ~TcpServer();

    /**
     * @brief 服务器绑定监听地址 虚函数
     *
     * @param addr 监听地址
     * @return true
     * @return false
     */
    virtual bool bind(ljrserver::Address::ptr addr);

    /**
     * @brief 服务器绑定多个监听地址 虚函数
     *
     * 有一个 bind 失败则返回 false
     *
     * @param addrs 要监听的地址数组
     * @param fails 监听失败的地址数组
     * @return true
     * @return false
     */
    virtual bool bind(const std::vector<Address::ptr> &addrs,
                      std::vector<Address::ptr> &fails);

    /**
     * @brief 启动 tcp 服务器 虚函数
     *
     * @return true
     * @return false
     */
    virtual bool start();

    /**
     * @brief 停止 tcp 服务器 虚函数
     *
     */
    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }

    std::string getName() const { return m_name; }
    void setName(const std::string &v) { m_name = v; }

    bool isStop() const { return m_isStop; }

protected:
    /**
     * @brief 处理客户端的连接 虚函数
     *
     * @param client 连接成功的 socket 对象
     */
    virtual void handleClient(Socket::ptr client);

    /**
     * @brief 服务器开始接受客户端的 connect 连接 虚函数
     *
     * @param sock socket 对象指针
     */
    virtual void startAccept(Socket::ptr sock);

private:
    // socket 对象数组
    std::vector<Socket::ptr> m_socks;

    // 工作协程 处理客户端连接 handle connect
    IOManager *m_worker;

    // 等待接受客户端连接的协程 accept connect
    IOManager *m_acceptWorker;

    // 服务器接收超时
    uint64_t m_recvTimeout;

    // tcp 服务器名称
    std::string m_name;

    // 是否关闭
    bool m_isStop;
};

}  // namespace ljrserver

#endif  //__LJRSERVER_TCP_SERVER_H__