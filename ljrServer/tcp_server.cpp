
#include "tcp_server.h"
#include "config.h"
#include "log.h"

namespace ljrserver {

// 配置 tcp 服务器接收超时 2min
static ljrserver::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    ljrserver::Config::Lookup("tcp_server.read_timeout",
                              (uint64_t)(60 * 1000 * 2),
                              "tcp server read timeout");

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/**
 * @brief tcp 服务器的构造函数
 *
 * @param worker 工作协程 [= ljrserver::IOManager::GetThis()]
 * @param acceptworker 等待接受客户端连接的协程 [= ljrserver::IOManager::GetThis()]
 */
TcpServer::TcpServer(ljrserver::IOManager *worker,
                     ljrserver::IOManager *acceptworker)
    : m_worker(worker),
      m_acceptWorker(acceptworker),
      m_recvTimeout(g_tcp_server_read_timeout->getValue()),
      m_name("ljrserver/1.0.0"),
      m_isStop(true) {
    // 初始化列表要和成员变量声明顺序一致
}

/**
 * @brief tcp 服务器的析构函数 基类析构函数设置为虚函数
 *
 */
TcpServer::~TcpServer() {
    // 循环访问 socket 数组
    for (auto &sock : m_socks) {
        // 关闭 socket 对象
        sock->close();
    }
    // 清空
    m_socks.clear();
}

/**
 * @brief 服务器绑定监听地址 虚函数
 *
 * @param addr 监听地址
 * @return true
 * @return false
 */
bool TcpServer::bind(ljrserver::Address::ptr addr) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

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
bool TcpServer::bind(const std::vector<Address::ptr> &addrs,
                     std::vector<Address::ptr> &fails) {
    // 循环访问要监听的地址
    for (auto &addr : addrs) {
        // 根据地址的协议簇创建 tcp socket
        Socket::ptr sock = Socket::CreateTCP(addr);

        // 绑定地址
        if (!sock->bind(addr)) {
            // 绑定失败
            LJRSERVER_LOG_ERROR(g_logger)
                << "bind fail errno = " << errno
                << " errno-string = " << strerror(errno) << " addr = ["
                << addr->toString() << "]";
            // 加入失败数组
            fails.push_back(addr);
            continue;
        }

        // 监听地址
        if (!sock->listen()) {
            // 监听失败
            LJRSERVER_LOG_ERROR(g_logger)
                << "listen fail errno = " << errno
                << " errno-string = " << strerror(errno) << " addr = ["
                << addr->toString() << "]";
            // 加入失败数组
            fails.push_back(addr);
            continue;
        }

        // bind listen 成功的 socket 加入数组
        m_socks.push_back(sock);
    }

    if (!fails.empty()) {
        // 有连接失败的 清空
        m_socks.clear();
        // 返回 bind 失败
        return false;
    }

    // 成功
    for (auto &sock : m_socks) {
        LJRSERVER_LOG_INFO(g_logger) << "server bind success: " << *sock;
    }
    return true;
}

/**
 * @brief 启动 tcp 服务器 虚函数
 *
 * @return true
 * @return false
 */
bool TcpServer::start() {
    if (!m_isStop) {
        // 已经启动了
        return true;
    }
    // 标识启动
    m_isStop = false;

    // 循环访问 socket 对象
    for (auto &sock : m_socks) {
        // 开始接收连接 并处理
        m_acceptWorker->schedule(
            std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }
    return true;
}

/**
 * @brief 停止 tcp 服务器 虚函数
 *
 */
void TcpServer::stop() {
    // 标识停止
    m_isStop = true;

    // 获取自身对象
    auto self = shared_from_this();
    // 调度任务
    m_acceptWorker->schedule([this, self]() {
        // 循环访问 socket 对象
        for (auto &sock : m_socks) {
            // 取消所有事件
            sock->cancelAll();
            // 关闭 socket
            sock->close();
        }
        // 清空
        m_socks.clear();
    });
}

/**
 * @brief 处理客户端的连接 虚函数
 *
 * @param client 连接成功的 socket 对象
 */
void TcpServer::handleClient(Socket::ptr client) {
    LJRSERVER_LOG_INFO(g_logger) << "handleClient: " << *client;
}

/**
 * @brief 服务器开始接受客户端的 connect 连接 虚函数
 *
 * @param sock socket 对象指针
 */
void TcpServer::startAccept(Socket::ptr sock) {
    // 只要 tcp 服务器没有关闭
    while (!m_isStop) {
        // 等待接受客户端 connect 连接
        Socket::ptr client = sock->accept();

        if (client) {
            // 连接成功
            // 设置客户端接收超时
            client->setRecvTimeout(m_recvTimeout);
            // 处理连接
            m_worker->schedule(std::bind(&TcpServer::handleClient,
                                         shared_from_this(), client));
        } else {
            // 连接失败
            LJRSERVER_LOG_ERROR(g_logger)
                << "accept errno = " << errno
                << " errno-string = " << strerror(errno);
        }
    }
}

}  // namespace ljrserver
