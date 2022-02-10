
#ifndef __LJRSERVER_SOCKET_H__
#define __LJRSERVER_SOCKET_H__

// 智能指针
#include <memory>

// 地址
#include "address.h"
// 不可复制
#include "noncopyable.h"

namespace ljrserver {

/**
 * @brief Class 封装 socket 对象
 *
 * 继承自 enable_shared_from_this Noncopyable
 * 不可复制
 */
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    // 智能指针
    typedef std::shared_ptr<Socket> ptr;

    // 弱引用
    typedef std::weak_ptr<Socket> weak_ptr;

    // socket TCP/UDP 类型
    enum Type { TCP = SOCK_STREAM, UDP = SOCK_DGRAM };

    // IPv4/IPv6/UNIX 协议簇
    enum Family { IPv4 = AF_INET, IPv6 = AF_INET6, UNIX = AF_UNIX };

public:  /// 便利函数
    static Socket::ptr CreateTCP(ljrserver::Address::ptr address);
    static Socket::ptr CreateUDP(ljrserver::Address::ptr address);

    static Socket::ptr CreateTCPSocket();
    static Socket::ptr CreateUDPSocket();

    static Socket::ptr CreateTCPSocket6();
    static Socket::ptr CreateUDPSocket6();

    static Socket::ptr CreateUnixTCPSocket();
    static Socket::ptr CreateUnixUDPSocket();

public:  /// 构造析构
    /**
     * @brief socket 构造函数
     *
     * @param family
     * @param type
     * @param protocol
     */
    Socket(int family, int type, int protocol = 0);

    /**
     * @brief socket 析构函数
     *
     */
    ~Socket();

public:  /// 超时相关
    /**
     * @brief 获取发送超时
     *
     * @return int64_t 毫秒
     */
    int64_t getSendTimeout();

    /**
     * @brief 设置发送超时
     *
     * @param v 毫秒
     */
    void setSendTimeout(int64_t v);

    /**
     * @brief 获取接收超时
     *
     * @return int64_t 毫秒
     */
    int64_t getRecvTimeout();

    /**
     * @brief 设置接收超时
     *
     * @param v 毫秒
     */
    void setRecvTimeout(int64_t v);

public:  /// 句柄
    /**
     * @brief 获取 sockopt
     *
     */
    bool getOption(int level, int option, void *result, size_t *len);
    template <class T>
    bool getOption(int level, int option, T &result) {
        // size_t length = sizeof(T);
        return getOption(level, option, &result, sizeof(T));
    }

    /**
     * @brief 设置 sockopt
     *
     */
    bool setOption(int level, int option, const void *result, size_t len);
    template <class T>
    bool setOption(int level, int option, const T &result) {
        return setOption(level, option, &result, sizeof(T));
    }

public:  /// 连接
    /**
     * @brief 服务器接受 connect 连接
     * @pre Socket 必须 bind listen 成功
     * @return Socket::ptr 成功返回新连接的 socket 失败返回 nullptr
     */
    Socket::ptr accept();

    /**
     * @brief 服务器绑定一个监听地址
     *
     * @param addr 地址
     * @return true 绑定成功
     * @return false 绑定失败
     */
    bool bind(const Address::ptr addr);

    /**
     * @brief 客户端连接服务器地址
     *
     * @param addr 服务器地址
     * @param timeout_ms 超时
     * @return true 连接成功
     * @return false 连接失败
     */
    bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    /**
     * @brief 服务器监听 socket 句柄
     * @pre 必须先 bind 成功
     * @param backlog 未完成连接队列的最大长度 [= 4096]
     * @return true 监听成功
     * @return false 监听失败
     */
    bool listen(int backlog = SOMAXCONN);

    /**
     * @brief 关闭 socket
     *
     * @return true
     * @return false
     */
    bool close();

public:  /// 发送
    int send(const void *buffer, size_t length, int flags = 0);
    int send(const iovec *buffers, size_t length, int flags = 0);
    int sendTo(const void *buffer, size_t length, const Address::ptr to,
               int flags = 0);
    int sendTo(const iovec *buffers, size_t length, const Address::ptr to,
               int flags = 0);

public:  /// 接收
    int recv(void *buffer, size_t length, int flags = 0);
    int recv(iovec *buffers, size_t length, int flags = 0);
    int recvFrom(void *buffer, size_t length, Address::ptr from, int flags = 0);
    int recvFrom(iovec *buffers, size_t length, Address::ptr from,
                 int flags = 0);

public:  /// 属性
    /**
     * @brief 获取远程地址 getpeername
     *
     * @return Address::ptr 地址虚基类指针
     */
    Address::ptr getRemoteAddress();

    /**
     * @brief 获取本机地址 getsockname
     *
     * @return Address::ptr 地址虚基类指针
     */
    Address::ptr getLocalAddress();

    int getSocket() const { return m_sock; }
    int getFamily() const { return m_family; }
    int getType() const { return m_type; }
    int getProtocol() const { return m_protocol; }
    bool isConnected() const { return m_isConnected; }

    /**
     * @brief 是否可用
     *
     * @return true
     * @return false
     */
    bool isValid() const;

    /**
     * @brief 获取 socket 的错误
     *
     * @return int 错误
     */
    int getError();

    /**
     * @brief 输出信息到流
     *
     * @param os
     * @return std::ostream&
     */
    std::ostream &dump(std::ostream &os) const;

public:  /// IO 事件管理
    /**
     * @brief 取消读事件
     *
     * @return true
     * @return false
     */
    bool cancelRead();

    /**
     * @brief 取消写事件
     *
     * @return true
     * @return false
     */
    bool cancelWrite();

    /**
     * @brief 取消接受 connect 连接
     *
     * @return true
     * @return false
     */
    bool cancelAccept();

    /**
     * @brief 取消全部事件
     *
     * @return true
     * @return false
     */
    bool cancelAll();

private:  // socket 初始化相关 private
    /**
     * @brief accept 初始化 socket
     * private
     *
     * @param sock socket 句柄
     * @return true
     * @return false
     */
    bool init(int sock);

    /**
     * @brief 初始化 socket
     * private
     */
    void initSock();

    /**
     * @brief 创建 socket
     *
     */
    void newSock();

private:
    // socket 句柄
    int m_sock;

    // 协议簇 (AF_INET, AF_INET6, AF_UNIX)
    int m_family;

    // tcp/udp 类型
    int m_type;

    // 协议
    int m_protocol;

    // 是否连接
    bool m_isConnected;

    // 本机地址
    Address::ptr m_localAddress;

    // 远程地址
    Address::ptr m_remoteAddress;
};

/**
 * @brief 流式输出 socket
 *
 * @param os 数据流
 * @param socket socket 对象
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const Socket &addr);

}  // namespace ljrserver

#endif  //__LJRSERVER_SOCKET_H__