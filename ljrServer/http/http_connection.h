
#ifndef __LJRSERVER_HTTP_CONNECTION_H__
#define __LJRSERVER_HTTP_CONNECTION_H__

// http 请求/响应 封装
#include "http.h"
// socket 流
#include "../socket_stream.h"
// 线程
#include "../thread.h"
// 工具函数
#include "../uri.h"
// 列表
#include <list>

namespace ljrserver {

namespace http {

/**
 * @brief Class 封装 http 请求结果
 *
 */
struct HttpResult {
    // 默认 public:
    // 智能指针
    typedef std::shared_ptr<HttpResult> ptr;

    /**
     * @brief 模版类
     * 服务器 http 响应的状态
     */
    enum class Error {
        OK = 0,
        INVALID_URL = 1,
        INVALID_HOST = 2,
        CONNECT_FAIL = 3,
        SEND_SLOSE_BY_PEER = 4,
        SEND_SOCKET_ERROR = 5,
        TIMEOUT = 6,
        CREATE_SOCKET_ERROR = 7,
        // 从连接池中取连接失败
        POOL_GET_CONNECTION = 8,
        POOL_INVALID_CONNECTION = 9,

    };

    /**
     * @brief http 请求结果的构造函数
     *
     * @param _result
     * @param _response
     * @param _error
     */
    HttpResult(int _result, HttpResponse::ptr _response,
               const std::string &_error)
        : result(_result), response(_response), error(_error) {
        // 初始化列表顺序与成员变量定义顺序一致
    }

    /**
     * @brief HttpResult 转字符串
     *
     * @return std::string
     */
    std::string toString() const;

    // 请求结果
    int result;

    // http 响应对象
    HttpResponse::ptr response;

    // 错误消息
    std::string error;
};

/// 声明 http 连接池
class HttpConnectionPool;

/**
 * @brief Class 封装 http connection 连接
 *
 * 继承自 SocketStream 实现 http 客户端
 *
 */
class HttpConnection : public SocketStream {
    // 友元类
    friend class HttpConnectionPool;

public:  // 默认私有 不能缺少 public 不然无法使用该别名
    // 智能指针
    typedef std::shared_ptr<HttpConnection> ptr;

public:  /// Request
    static HttpResult::ptr DoRequest(
        HttpMethod method, const std::string &url, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

    static HttpResult::ptr DoRequest(
        HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

    static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri,
                                     uint64_t timeout_ms);

public:  /// GET
    static HttpResult::ptr DoGet(
        const std::string &url, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

    static HttpResult::ptr DoGet(
        Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

public:  /// POST
    static HttpResult::ptr DoPost(
        const std::string &url, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

    static HttpResult::ptr DoPost(
        Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

public:
    /**
     * @brief 构造 http connection 连接
     *
     * @param sock socket 句柄对象
     * @param owner 是否持有 [= true]
     */
    HttpConnection(Socket::ptr sock, bool owner = true);

    /**
     * @brief 连接 析构函数
     *
     */
    ~HttpConnection();

    /**
     * @brief 客户端通过 socket stream 发送请求
     *
     * @param req http 请求对象
     * @return int
     */
    int sendRequest(HttpRequest::ptr req);

    /**
     * @brief 客户端接收请求响应
     *
     * @return HttpResponse::ptr
     */
    HttpResponse::ptr recvResponse();

private:
    // 连接创建时间
    uint64_t m_createTime = 0;

    uint64_t m_request = 0;
};

/**
 * @brief Class 封装 http 连接池
 *
 */
class HttpConnectionPool {
public:
    // 智能指针
    typedef std::shared_ptr<HttpConnectionPool> ptr;

    // 互斥锁
    typedef Mutex MutexType;

public:
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
    HttpConnectionPool(const std::string &host, const std::string &vhost,
                       int32_t port, int32_t max_size, uint32_t max_alive_time,
                       uint32_t max_request);

    /**
     * @brief 获取一个连接对象
     *
     * @return HttpConnection::ptr
     */
    HttpConnection::ptr getConnection();

public:  /// Request
    HttpResult::ptr doRequest(
        HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

    HttpResult::ptr doRequest(
        HttpMethod method, const std::string &url, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

    HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout_ms);

public:  /// GET
    HttpResult::ptr doGet(
        const std::string &url, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

    HttpResult::ptr doGet(
        Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

public:  /// POST
    HttpResult::ptr doPost(
        const std::string &url, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

    HttpResult::ptr doPost(
        Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string> &headers = {},
        const std::string &body = "");

private:
    /**
     * @brief 释放连接
     *
     * @param ptr
     * @param pool
     */
    static void ReleasePtr(HttpConnection *ptr, HttpConnectionPool *pool);

    // struct ConnectionInfo
    // {
    //     HttpConnection *conn;
    //     uint64_t create_time;
    // };

private:
    std::string m_host;

    std::string m_vhost;

    uint32_t m_port;

    uint32_t m_maxSize;

    uint32_t m_maxAliveTime;

    uint32_t m_maxRequest;

    // 互斥锁
    MutexType m_mutex;

    // 连接池 list
    std::list<HttpConnection *> m_conns;

    // 连接总数
    std::atomic<int32_t> m_total = {0};
};

}  // namespace http

}  // namespace ljrserver

#endif  //__LJRSERVER_HTTP_CONNECTION_H__