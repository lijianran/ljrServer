
#ifndef __LJRSERVER_SOCKET_STREAM_H__
#define __LJRSERVER_SOCKET_STREAM_H__

// socket 封装类
#include "socket.h"
// 流抽象类
#include "stream.h"

namespace ljrserver {

/**
 * @brief Class 封装 socket 流
 *
 * 继承自 Stream 流抽象类
 */
class SocketStream : public Stream {
public:
    // 智能指针
    typedef std::shared_ptr<SocketStream> ptr;

    /**
     * @brief socket 流构造函数
     *
     * @param sock socket 句柄对象
     * @param owner 析构是否释放 socket
     */
    SocketStream(Socket::ptr sock, bool owner = true);

    /**
     * @brief socket 流析构函数
     *
     */
    ~SocketStream();

public:  /// Stream 纯虚函数的实现
    /**
     * @brief 关闭 socket 流
     *
     */
    void close() override;

    int read(void *buffer, size_t length) override;
    int read(ByteArray::ptr ba, size_t length) override;

    int write(const void *buffer, size_t length) override;
    int write(ByteArray::ptr ba, size_t length) override;

public:
    /**
     * @brief 是否连接
     *
     * @return true
     * @return false
     */
    bool isConnected() const;

    /**
     * @brief 获取 socket 句柄对象
     *
     * @return Socket::ptr
     */
    Socket::ptr getSocket() const { return m_socket; }

protected:
    // socket 句柄对象
    Socket::ptr m_socket;

    // 析构是否释放socket
    bool m_owner;
};

}  // namespace ljrserver

#endif