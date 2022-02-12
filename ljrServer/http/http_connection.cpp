
#include "http_connection.h"
// http 报文解析
#include "http_parser.h"
// 日志
#include "../log.h"

// #include <strings.h>

namespace ljrserver {

namespace http {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/**
 * @brief HttpResult 转字符串
 *
 * @return std::string
 */
std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult result=" << result << " error=" << error
       << " response:" << std::endl
       << (response ? response->toString() : "nullptr") << "]";
    return ss.str();
}

/********************************************
 *  HttpConnection 连接
 ********************************************/
/**
 * @brief 构造 http connection 连接
 *
 * @param sock socket 句柄对象
 * @param owner 是否持有 [= true]
 */
HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner) {
    // 初始化构造 SocketStream 对象
}

/**
 * @brief 连接 析构函数
 *
 */
HttpConnection::~HttpConnection() {
    LJRSERVER_LOG_DEBUG(g_logger) << "HttpConnetion::~HttpConnetion";
}

/**
 * @brief 客户端通过 socket stream 发送请求
 *
 * @param req http 请求对象
 * @return int
 */
int HttpConnection::sendRequest(HttpRequest::ptr req) {
    std::stringstream ss;
    // 输出 http 请求报文
    ss << *req;
    std::string data = ss.str();
    // 通过继承方法 Stream::writeFixSize 传输 http 请求报文
    return writeFixSize(data.c_str(), data.size());
}

/**
 * @brief 客户端接收请求响应
 *
 * @return HttpResponse::ptr
 */
HttpResponse::ptr HttpConnection::recvResponse() {
    // http 响应解析器
    HttpResponseParser::ptr parser(new HttpResponseParser);
    // 获取约定好的配置属性 http 请求缓存大小
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    // 缓存
    std::shared_ptr<char> buffer(new char[buff_size + 1], [](char *ptr) {
        // 自定义析构方法
        delete[] ptr;
    });

    // 裸指针
    char *data = buffer.get();
    // 缓存偏移 数据量大小
    int offset = 0;
    // 循环接收响应
    do {
        // 通过 SocketStream::read 读取
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            // 没有数据 关闭 SocketStream::close
            close();
            return nullptr;
        }

        len += offset;
        data[len] = '\0';
        // 解析 http 响应内容
        size_t nparse = parser->execute(data, len, false);
        if (parser->hasError()) {
            // 有错误 关闭 SocketStream::close
            close();
            return nullptr;
        }

        offset = len - nparse;
        if (offset == (int)buff_size) {
            // 缓存已经满了
            close();
            return nullptr;
        }

        if (parser->isFinished()) {
            // 解析已经完成
            break;
        }

    } while (true);

    // 获取响应解析器
    auto &client_parser = parser->getParser();
    // 响应体
    std::string body;

    if (client_parser.chunked) {
        // 如果分块了
        int len = offset;
        do {
            // 循环接收数据
            do {
                int rt = read(data + len, buff_size - len);
                if (rt <= 0) {
                    // 接收失败
                    close();
                    return nullptr;
                }

                len += rt;
                data[len] = '\0';
                // 解析内容
                size_t nparse = parser->execute(data, len, true);
                if (parser->hasError()) {
                    // 有错误
                    close();
                    return nullptr;
                }

                len -= nparse;

                if (len == (int)buff_size) {
                    // 缓存满了
                    close();
                    return nullptr;
                }

            } while (!parser->isFinished());
            // 解析完毕

            len -= 2;

            LJRSERVER_LOG_INFO(g_logger)
                << "content_len=" << client_parser.content_len;

            if (client_parser.content_len <= len) {
                body.append(data, client_parser.content_len);

                memmove(data, data + client_parser.content_len,
                        len - client_parser.content_len);

                len -= client_parser.content_len;

            } else {
                body.append(data, len);
                int left = client_parser.content_len - len;

                while (left > 0) {
                    int rt = read(
                        data, left > (int)buff_size ? (int)buff_size : left);
                    if (rt <= 0) {
                        close();
                        return nullptr;
                    }

                    body.append(data, rt);
                    left -= rt;
                }

                len = 0;
            }
        } while (!client_parser.chunks_done);
        // 分块结束

        // 设置 http 响应对象的内容体
        parser->getData()->setBody(body);

    } else {
        // 没有分块 获取 content-length
        int64_t length = parser->getContentLength();
        if (length > 0) {
            // 缓存
            std::string body;
            body.resize(length);

            int len = 0;
            if (length >= offset) {
                // body.append(data, offset);
                memcpy(&body[0], data, offset);
                len = offset;
            } else {
                // body.append(data, length);
                memcpy(&body[0], data, length);
                len = length;
            }

            length -= offset;
            if (length > 0) {
                // 还没接收完成 调用 Stream::readFixSize 继续接受剩下数据
                if (readFixSize(&body[len], length) <= 0) {
                    // 接收失败 关闭
                    close();
                    return nullptr;
                }
            }

            // 设置 http 响应对象的内容体
            parser->getData()->setBody(body);
        }
    }

    // 返回 http 响应对象
    return parser->getData();
}

/// Requset

HttpResult::ptr HttpConnection::DoRequest(
    HttpMethod method, const std::string &url, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // URI 资源
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        // 返回错误结果 INVALID_URLƒ
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,
                                            nullptr, "invalid url: " + url);
    }

    // 发送请求
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(
    HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 构造 http 请求对象
    HttpRequest::ptr req = std::make_shared<HttpRequest>();

    // 设置请求头部
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);

    bool has_host = false;
    for (auto &i : headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0) {
            // 是否长连接
            if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        // 是否有 host
        if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        // 设置头部
        req->setHeader(i.first, i.second);
    }

    if (!has_host) {
        // 设置 host
        req->setHeader("Host", uri->getHost());
    }

    // 设置请求体
    req->setBody(body);
    // 发送请求
    return DoRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req, Uri::ptr uri,
                                          uint64_t timeout_ms) {
    // 创建地址
    Address::ptr addr = uri->createAddress();
    if (!addr) {
        // 创建失败
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::INVALID_HOST, nullptr,
            "invalid host: " + uri->getHost());
    }

    // 根据本机地址协议簇创建 socket 对象
    Socket::ptr sock = Socket::CreateTCP(addr);
    if (!sock) {
        // 创建失败
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::CREATE_SOCKET_ERROR, nullptr,
            "create socket fail: " + addr->toString() +
                " errno=" + std::to_string(errno) +
                " errno-string=" + std::string(strerror(errno)));
    }

    // 创建连接
    if (!sock->connect(addr)) {
        // 连接失败
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::CONNECT_FAIL, nullptr,
            "sock connect fail: " + addr->toString());
    }
    // 设置接受超时
    sock->setRecvTimeout(timeout_ms);

    // 构造连接对象
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    // 发送请求
    int rt = conn->sendRequest(req);
    if (rt == 0) {
        // 发送失败 SEND_SLOSE_BY_PEER 服务器关闭 socket
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::SEND_SLOSE_BY_PEER, nullptr,
            "send request closed by peer: " + addr->toString());
    }

    if (rt < 0) {
        // 其他错误
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
            "send request socket error errno=" + std::to_string(errno) +
                " errno-string=" + std::string(strerror(errno)));
    }

    // 接收响应
    auto rsp = conn->recvResponse();
    if (!rsp) {
        // 接收失败 TIMEOUT 超时错误
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::TIMEOUT, nullptr,
            "recv request timeout: " + addr->toString() +
                " timeout_ms: " + std::to_string(timeout_ms));
    }

    // 发送成功 返回响应结果 200 OK
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
}

/// GET

HttpResult::ptr HttpConnection::DoGet(
    const std::string &url, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 创建 URL
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        // 创建失败
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,
                                            nullptr, "invalid url: " + url);
    }
    // 发送 get 请求
    return DoGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(
    Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 发送请求
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

/// POST

HttpResult::ptr HttpConnection::DoPost(
    const std::string &url, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 创建 URI
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        // 创建失败
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,
                                            nullptr, "invalid url: " + url);
    }
    // 发送 post 请求
    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(
    Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 发送请求
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

/********************************************
 *  HttpConnectionPool 连接池
 ********************************************/

/**
 * @brief 连接池构造函数
 *
 * @param host
 * @param vhost
 * @param port
 * @param max_size
 * @param max_alive_time
 * @param max_request
 */
HttpConnectionPool::HttpConnectionPool(const std::string &host,
                                       const std::string &vhost, int32_t port,
                                       int32_t max_size,
                                       uint32_t max_alive_time,
                                       uint32_t max_request)
    : m_host(host),
      m_vhost(vhost),
      m_port(port),
      m_maxSize(max_size),
      m_maxAliveTime(max_alive_time),
      m_maxRequest(max_request) {
    // 初始化列表顺序必须与成员变量声明顺序一致
}

/**
 * @brief 释放连接
 *
 * @param ptr
 * @param pool
 */
void HttpConnectionPool::ReleasePtr(HttpConnection *ptr,
                                    HttpConnectionPool *pool) {
    ++ptr->m_request;

    if (!ptr->isConnected() ||
        (ptr->m_createTime + pool->m_maxAliveTime) >=
            ljrserver::GetCurrentMS() ||
        (ptr->m_request >= pool->m_maxRequest)) {
        // 没有连接
        // 超时
        // 请求数满了
        delete ptr;
        --pool->m_total;
        return;
    }

    // 上锁
    MutexType::Lock lock(pool->m_mutex);
    // 加入连接池
    pool->m_conns.push_back(ptr);
}

/**
 * @brief 获取一个连接对象
 *
 * @return HttpConnection::ptr
 */
HttpConnection::ptr HttpConnectionPool::getConnection() {
    // 获取当前时间
    uint64_t now_ms = ljrserver::GetCurrentMS();

    // 失效连接数组
    std::vector<HttpConnection *> invalid_conns;
    // 连接对象
    HttpConnection *ptr = nullptr;

    // 上锁
    MutexType::Lock lock(m_mutex);
    while (!m_conns.empty()) {
        // 取出头部
        auto conn = *m_conns.begin();
        m_conns.pop_front();

        if (!conn->isConnected()) {
            // 没有连接 加入失效连接数组
            invalid_conns.push_back(conn);
            continue;
        }

        if ((conn->m_createTime + m_maxAliveTime) > now_ms) {
            // 超过最大存活时间 链接失效了
            invalid_conns.push_back(conn);
            continue;
        }

        // 找到一个好的连接
        ptr = conn;
        break;
    }
    // 解锁
    lock.unlock();

    for (auto i : invalid_conns) {
        // 删除失效的连接
        delete i;
    }

    // 更新总数
    m_total -= invalid_conns.size();

    // 是否有找到连接
    if (!ptr) {
        // 没有找到连接则新建连接
        // 根据本机创建地址
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if (!addr) {
            // 创建失败
            LJRSERVER_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }

        // 设置端口
        addr->setPort(m_port);
        // 根据地址的协议簇创建 tcp 连接
        Socket::ptr sock = Socket::CreateTCP(addr);
        if (!sock) {
            // 创建失败
            LJRSERVER_LOG_ERROR(g_logger) << "create sock fail: " << *addr;
            return nullptr;
        }
        // 连接地址
        if (!sock->connect(addr)) {
            // 连接失败
            LJRSERVER_LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
            return nullptr;
        }

        // 构造 HttpConnection 连接对象
        ptr = new HttpConnection(sock);
        // 总数 +1
        ++m_total;
    }

    // std::placeholders::_1
    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr,
                                              std::placeholders::_1, this));
}

/// Request

HttpResult::ptr HttpConnectionPool::doRequest(
    HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 缓存
    std::stringstream ss;
    // 输出 URI
    ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery() << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    // 发送请求
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(
    HttpMethod method, const std::string &url, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 构造 http 请求对象
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url);
    req->setMethod(method);
    // 设置非长连接
    req->setClose(false);

    bool has_host = false;
    for (auto &i : headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0) {
            if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        req->setHeader(i.first, i.second);
    }

    if (!has_host) {
        if (m_vhost.empty()) {
            req->setHeader("Host", m_host);
        } else {
            req->setHeader("Host", m_vhost);
        }
    }

    req->setBody(body);
    return doRequest(req, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req,
                                              uint64_t timeout_ms) {
    // 获取一个可用连接
    auto conn = getConnection();
    if (!conn) {
        // 获取失败
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::POOL_GET_CONNECTION, nullptr,
            "getconnection fail, pool host: " + m_host +
                " port: " + std::to_string(m_port));
    }

    // 获取该可用连接的 socket 句柄对象
    auto sock = conn->getSocket();
    if (!sock) {
        // 获取失败
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::POOL_INVALID_CONNECTION, nullptr,
            "getSocket fail, pool host: " + m_host +
                " port: " + std::to_string(m_port));
    }

    // 设置接收超时
    sock->setRecvTimeout(timeout_ms);

    // 构造连接对象
    // HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    // 通过连接池中的可用连接发送请求
    int rt = conn->sendRequest(req);
    if (rt == 0) {
        // 发送失败
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::SEND_SLOSE_BY_PEER, nullptr,
            "send request closed by peer: " +
                sock->getRemoteAddress()->toString());
    }

    if (rt < 0) {
        // 其他错误
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
            "send request socket error errno = " + std::to_string(errno) +
                " errno-string = " + std::string(strerror(errno)));
    }

    // 接收响应
    auto rsp = conn->recvResponse();
    if (!rsp) {
        // 接收失败
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::TIMEOUT, nullptr,
            "recv request timeout: " + sock->getRemoteAddress()->toString() +
                " timeout_ms: " + std::to_string(timeout_ms));
    }

    // 成功返回 HttpResult 响应结果 200 OK
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
}

/// GET

HttpResult::ptr HttpConnectionPool::doGet(
    const std::string &url, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 发送 get 请求
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(
    Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 输出 URI
    std::stringstream ss;
    ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery() << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    // 发送 get 请求
    return doGet(ss.str(), timeout_ms, headers, body);
}

/// POST

HttpResult::ptr HttpConnectionPool::doPost(
    const std::string &url, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 发送 post 请求
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(
    Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers,
    const std::string &body) {
    // 输出 RUI
    std::stringstream ss;
    ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery() << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    // 发送 post 请求
    return doPost(ss.str(), timeout_ms, headers, body);
}

}  // namespace http

}  // namespace ljrserver
