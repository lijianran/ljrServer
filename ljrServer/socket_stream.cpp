
#include "socket_stream.h"

namespace ljrserver {

/**
 * @brief socket 流构造函数
 *
 * @param sock socket 句柄对象
 * @param owner 析构是否释放 socket
 */
SocketStream::SocketStream(Socket::ptr sock, bool owner)
    : m_socket(sock), m_owner(owner) {}

/**
 * @brief socket 流析构函数
 *
 */
SocketStream::~SocketStream() {
    if (m_owner && m_socket) {
        m_socket->close();
    }
}

/**
 * @brief 关闭 socket 流
 *
 */
void SocketStream::close() {
    if (m_socket) {
        m_socket->close();
    }
}

/**
 *
 * @brief 读取数据到 buffer 纯虚函数的实现
 *
 * @param buffer
 * @param length
 * @return int
 */
int SocketStream::read(void *buffer, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    return m_socket->recv(buffer, length);
}

/**
 * @brief 读取数据到 byte array 纯虚函数的实现
 *
 * @param ba
 * @param length
 * @return int
 */
int SocketStream::read(ByteArray::ptr ba, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    // 创建缓存
    std::vector<iovec> iovs;
    // 不修改 m_position
    ba->getWriteBuffers(iovs, length);

    // 接收数据
    int rt = m_socket->recv(&iovs[0], iovs.size());
    if (rt > 0) {
        // 接收到了数据 设置 m_position
        ba->setPostion(ba->getPosition() + rt);
    }

    // 返回接收到的数据个数
    return rt;
}

/**
 * @brief 写入 buffer 中的数据 纯虚函数的实现
 *
 * @param ba
 * @param length
 * @return int
 */
int SocketStream::write(const void *buffer, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    return m_socket->send(buffer, length);
}

/**
 * @brief 写入 byte array 中的数据 纯虚函数的实现
 *
 * @param ba
 * @param length
 * @return int
 */
int SocketStream::write(ByteArray::ptr ba, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    // 创建缓存
    std::vector<iovec> iovs;
    // 读取数据 不修改 m_position
    ba->getReadBuffers(iovs, length);

    // 发送数据
    int rt = m_socket->send(&iovs[0], iovs.size());
    if (rt > 0) {
        // 发送数据成功 修改 m_position
        ba->setPostion(ba->getPosition() + rt);
    }

    // 返回写入成功的数据长度
    return rt;
}

/**
 * @brief 是否连接
 *
 * @return true
 * @return false
 */
bool SocketStream::isConnected() const {
    return m_socket && m_socket->isConnected();
}

}  // namespace ljrserver
