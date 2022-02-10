
#include "socket.h"

// IO 管理
#include "iomanager.h"
// 句柄管理
#include "fd_manager.h"
// 日志
#include "log.h"
// 断言
#include "macro.h"
// hook
#include "hook.h"

// #include <limits.h>
// TCP_NODELAY 不延迟发
#include <netinet/tcp.h>

namespace ljrserver {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/******************************************
 * 便利函数
 ******************************************/

Socket::ptr Socket::CreateTCP(ljrserver::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDP(ljrserver::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

/******************************************
 * 构造析构
 ******************************************/

/**
 * @brief socket 构造函数
 *
 * @param family
 * @param type
 * @param protocol
 */
Socket::Socket(int family, int type, int protocol)
    : m_sock(-1),
      m_family(family),
      m_type(type),
      m_protocol(protocol),
      m_isConnected(false) {}

/**
 * @brief socket 析构函数
 *
 */
Socket::~Socket() {
    // 关闭 socket 句柄
    close();
}

/******************************************
 * 超时相关
 ******************************************/

/**
 * @brief 获取发送超时
 *
 * @return int64_t 毫秒
 */
int64_t Socket::getSendTimeout() {
    // 获取句柄对象
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) {
        // 获取发送超时
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    // 永久等待
    return -1;
}

/**
 * @brief 设置发送超时
 *
 * @param v 毫秒
 */
void Socket::setSendTimeout(int64_t v) {
    // 超时结构体
    struct timeval tv {
        int(v / 1000), int(v % 1000 * 1000)
    };
    // 设置发送超时
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

/**
 * @brief 获取接收超时
 *
 * @return int64_t 毫秒
 */
int64_t Socket::getRecvTimeout() {
    // 获取句柄对象
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) {
        // 获取接收超时
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    // 永久等待
    return -1;
}

/**
 * @brief 设置接收超时
 *
 * @param v 毫秒
 */
void Socket::setRecvTimeout(int64_t v) {
    // 超时结构体
    struct timeval tv {
        int(v / 1000), int(v % 1000 * 1000)
    };
    // 设置发送超时
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

/******************************************
 * 句柄
 ******************************************/

/**
 * @brief 获取 sockopt
 *
 */
bool Socket::getOption(int level, int option, void *result, size_t *len) {
    int rt = getsockopt(m_sock, level, option, result, (socklen_t *)len);
    if (rt) {
        LJRSERVER_LOG_DEBUG(g_logger)
            << "getOption sock = " << m_sock << " level = " << level
            << " option = " << option << " errno = " << errno
            << " errno-string = " << strerror(errno);
        return false;
    }
    return true;
}

/**
 * @brief 设置 sockopt
 *
 */
bool Socket::setOption(int level, int option, const void *result, size_t len) {
    if (setsockopt(m_sock, level, option, result, (socklen_t)len)) {
        LJRSERVER_LOG_DEBUG(g_logger)
            << "setOption sock = " << m_sock << " level = " << level
            << " option = " << option << " errno = " << errno
            << " errno-string = " << strerror(errno);
        return false;
    }
    return true;
}

/******************************************
 * 连接
 ******************************************/

/**
 * @brief 服务器接受 connect 连接
 * @pre Socket 必须 bind listen 成功
 * @return Socket::ptr 成功返回新连接的 socket 失败返回 nullptr
 */
Socket::ptr Socket::accept() {
    // 创建 socket 对象
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));

    // 接收 connect
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if (newsock == -1) {
        // 失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "accept(" << m_sock << ") errno = " << errno
            << " errno-string = " << strerror(errno);
        return nullptr;
    }
    // 初始化 socket
    if (sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

/**
 * @brief 服务器绑定一个监听地址
 *
 * @param addr 地址
 * @return true 绑定成功
 * @return false 绑定失败
 */
bool Socket::bind(const Address::ptr addr) {
    // 是否失效
    if (!isValid()) {
        // 失效则重新创建
        newSock();
        if (LJRSERVER_UNLICKLY(!isValid())) {
            // 重新创建失败
            return false;
        }
    }

    // 检查协议簇
    if (LJRSERVER_UNLICKLY(addr->getFamily() != m_family)) {
        LJRSERVER_LOG_ERROR(g_logger)
            << "bind sock.family(" << m_family << ") addr.family("
            << addr->getFamily() << ") not equal, addr = " << addr->toString();
        return false;
    }

    // 绑定地址 Address::ptr 动态多态
    if (::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        LJRSERVER_LOG_ERROR(g_logger) << "bind error errno = " << errno
                                      << " errno-string = " << strerror(errno);
        return false;
    }

    // 获取本机地址
    getLocalAddress();

    return true;
}

/**
 * @brief 客户端连接服务器地址
 *
 * @param addr 服务器地址
 * @param timeout_ms 超时
 * @return true 连接成功
 * @return false 连接失败
 */
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    // 是否失效
    if (!isValid()) {
        // 失效则重新创建
        newSock();
        if (LJRSERVER_UNLICKLY(!isValid())) {
            // 重新创建失败
            return false;
        }
    }

    // 检查协议簇
    if (LJRSERVER_UNLICKLY(addr->getFamily() != m_family)) {
        LJRSERVER_LOG_ERROR(g_logger)
            << "connect sock.family(" << m_family << ") addr.family("
            << addr->getFamily() << ") not equal, addr = " << addr->toString();
        return false;
    }

    // 检查超时
    if (timeout_ms == (uint64_t)-1) {
        // connect 永久超时
        if (::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            // 失败
            LJRSERVER_LOG_ERROR(g_logger)
                << "sock = " << m_sock << " connect(" << addr->toString()
                << ") error errno = " << errno
                << " errno-string = " << strerror(errno);
            // 关闭 socket
            close();
            return false;
        }
    } else {
        // connect 带超时
        if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(),
                                   timeout_ms)) {
            // 失败
            LJRSERVER_LOG_ERROR(g_logger)
                << "sock = " << m_sock << " connect(" << addr->toString()
                << ") timeout = " << timeout_ms << " error errno = " << errno
                << " errno-string = " << strerror(errno);
            // 关闭 socket
            close();
            return false;
        }
    }

    // 连接成功
    m_isConnected = true;
    // 获取远程地址
    getRemoteAddress();
    // 获取本机地址
    getLocalAddress();

    return true;
}

/**
 * @brief 服务器监听 socket 句柄
 * @pre 必须先 bind 成功
 * @param backlog 未完成连接队列的最大长度 [= 4096]
 * @return true 监听成功
 * @return false 监听失败
 */
bool Socket::listen(int backlog) {
    // 是否失效
    if (!isValid()) {
        LJRSERVER_LOG_ERROR(g_logger) << "listen error sock = -1";
        return false;
    }

    // listen 监听未 hook
    if (::listen(m_sock, backlog)) {
        LJRSERVER_LOG_ERROR(g_logger) << "listen error errno = " << errno
                                      << " errno-string = " << strerror(errno);
        return false;
    }
    return true;
}

/**
 * @brief 关闭 socket
 *
 * @return true
 * @return false
 */
bool Socket::close() {
    // 没有连接成功 socket 句柄无效
    if (!m_isConnected && m_sock == -1) {
        return true;
    }

    // 关闭连接
    m_isConnected = false;
    if (m_sock != -1) {
        // 关闭
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}

/******************************************
 * 发送
 ******************************************/

int Socket::send(const void *buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::send(const iovec *buffers, size_t length, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::sendTo(const void *buffer, size_t length, const Address::ptr to,
                   int flags) {
    if (isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(),
                        to->getAddrLen());
    }
    return -1;
}

int Socket::sendTo(const iovec *buffers, size_t length, const Address::ptr to,
                   int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

/******************************************
 * 接收
 ******************************************/

int Socket::recv(void *buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::recv(iovec *buffers, size_t length, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void *buffer, size_t length, Address::ptr from,
                     int flags) {
    if (isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

int Socket::recvFrom(iovec *buffers, size_t length, Address::ptr from,
                     int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

/******************************************
 * 属性
 ******************************************/

/**
 * @brief 获取远程地址 getpeername
 *
 * @return Address::ptr 地址虚基类指针
 */
Address::ptr Socket::getRemoteAddress() {
    if (m_remoteAddress) {
        return m_remoteAddress;
    }

    Address::ptr result;
    // 协议簇
    switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnkonwnAddress(m_family));
            break;
    }

    socklen_t addrlen = result->getAddrLen();
    // getpeername 获取远程地址
    if (getpeername(m_sock, result->getAddr(), &addrlen)) {
        // 失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "getpeername error sock = " << m_sock << " errno = " << errno
            << " errno-string = " << strerror(errno);
        // 返回 UnkonwnAddress
        return Address::ptr(new UnkonwnAddress(m_family));
    }

    if (m_family == AF_UNIX) {
        // Unix 地址
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }

    // 设置远程地址
    m_remoteAddress = result;

    return m_remoteAddress;
}

/**
 * @brief 获取本机地址 getsockname
 *
 * @return Address::ptr 地址虚基类指针
 */
Address::ptr Socket::getLocalAddress() {
    if (m_localAddress) {
        return m_localAddress;
    }

    Address::ptr result;
    // 协议簇
    switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnkonwnAddress(m_family));
            break;
    }

    socklen_t addrlen = result->getAddrLen();
    // getsockname 获取本机地址
    if (getsockname(m_sock, result->getAddr(), &addrlen)) {
        // 失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "getsockname error sock = " << m_sock << " errno = " << errno
            << " errno-string = " << strerror(errno);
        // 返回 UnkonwnAddress
        return Address::ptr(new UnkonwnAddress(m_family));
    }

    if (m_family == AF_UNIX) {
        // Unix 地址
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }

    // 设置本机地址
    m_localAddress = result;

    return m_localAddress;
}

/**
 * @brief 是否可用
 *
 * @return true
 * @return false
 */
bool Socket::isValid() const { return m_sock != -1; }

/**
 * @brief 获取 socket 的错误
 *
 * @return int 错误
 */
int Socket::getError() {
    int error = 0;
    size_t len = sizeof(error);
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    return error;
}

/**
 * @brief 输出信息到流
 *
 * @param os
 * @return std::ostream&
 */
std::ostream &Socket::dump(std::ostream &os) const {
    // socket 信息
    os << "[Socket sock=" << m_sock << " is_connected=" << m_isConnected
       << " family=" << m_family << " type=" << m_type
       << " protocol=" << m_protocol;
    // 本机地址
    if (m_localAddress) {
        os << " local_address = " << m_localAddress->toString();
    }
    // 远程地址
    if (m_remoteAddress) {
        os << " remote_address = " << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

/******************************************
 * IO 事件管理
 ******************************************/

/**
 * @brief 取消读事件
 *
 * @return true
 * @return false
 */
bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(m_sock,
                                             ljrserver::IOManager::READ);
}

/**
 * @brief 取消写事件
 *
 * @return true
 * @return false
 */
bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(m_sock,
                                             ljrserver::IOManager::WRITE);
}

/**
 * @brief 取消接受 connect 连接
 *
 * @return true
 * @return false
 */
bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(m_sock,
                                             ljrserver::IOManager::READ);
}

/**
 * @brief 取消全部事件
 *
 * @return true
 * @return false
 */
bool Socket::cancelAll() {
    // 会执行事件
    return IOManager::GetThis()->cancelAll(m_sock);
}

/******************************************
 * socket 初始化相关 private
 ******************************************/

/**
 * @brief accept 初始化 socket
 * private
 *
 * @param sock socket 句柄
 * @return true
 * @return false
 */
bool Socket::init(int sock) {
    // 获取句柄对象
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    // 获取成功且是 socket 且没有关闭
    if (ctx && ctx->isSocket() && !ctx->isClosed()) {
        // 设置 socket 句柄
        m_sock = sock;
        // 连接成功
        m_isConnected = true;

        // 初始化 socket
        initSock();
        // 获取本机地址
        getLocalAddress();
        // 获取远程地址
        getRemoteAddress();

        return true;
    }
    return false;
}

/**
 * @brief 初始化 socket
 * private
 */
void Socket::initSock() {
    int val = 1;
    // 设置
    setOption(SOL_SOCKET, SO_REUSEADDR, val);

    // tcp 类型
    if (m_type == SOCK_STREAM) {
        // TCP_NODELAY 不延迟发
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

/**
 * @brief 创建 socket
 *
 */
void Socket::newSock() {
    // 创建 socket 句柄
    m_sock = socket(m_family, m_type, m_protocol);

    if (LJRSERVER_LICKLY(m_sock != -1)) {
        // 创建成功 初始化
        initSock();
    } else {
        // 创建失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "socket(" << m_family << ", " << m_type << ", " << m_protocol
            << ") errno = " << errno << " errno-string = " << strerror(errno);
    }
}

/**
 * @brief 流式输出 socket
 *
 * @param os 数据流
 * @param socket socket 对象
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const Socket &socket) {
    // 调用 dump
    return socket.dump(os);
}

}  // namespace ljrserver
