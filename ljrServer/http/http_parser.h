
#ifndef __LJRSERVER_HTTP_PARSER_H__
#define __LJRSERVER_HTTP_PARSER_H__

// http 请求/响应
#include "http.h"
// http 请求解析
#include "http11_parser.h"
// http 响应解析
#include "httpclient_parser.h"

namespace ljrserver {

namespace http {

/**
 * @brief Class 解析 http 请求
 *
 */
class HttpRequestParser {
public:
    // 智能指针
    typedef std::shared_ptr<HttpRequestParser> ptr;

    /**
     * @brief http 请求解析器的构造函数
     *
     */
    HttpRequestParser();

public:
    /**
     * @brief 解析 http 请求
     *
     * @param data
     * @param len
     * @return size_t
     */
    size_t execute(char *data, size_t len);

    /**
     * @brief 是否解析完成
     *
     * @return int
     */
    int isFinished();

    /**
     * @brief 是否有错误
     *
     * @return int
     */
    int hasError();

    /**
     * @brief 获取 content-length
     *
     * @return uint64_t
     */
    uint64_t getContentLength();

    /**
     * @brief 获取解析的 http 请求对象
     *
     * @return HttpRequest::ptr
     */
    HttpRequest::ptr getData() const { return m_data; }

    /**
     * @brief 获取 http 请求解析器
     *
     * @return const http_parser&
     */
    const http_parser &getParser() const { return m_parser; }

    /**
     * @brief 设置错误
     *
     * @param v
     */
    void setError(int v) { m_error = v; }

public:
    static uint64_t GetHttpRequestBufferSize();

    static uint64_t GetHttpRequestMaxBodySize();

private:
    // http 请求解析器
    http_parser m_parser;

    // http 请求对象
    HttpRequest::ptr m_data;

    // 1000: invalid method
    // 1001: invalid version
    // 1002: invalid field
    int m_error;
};

/**
 * @brief Class 解析 http 响应
 *
 */
class HttpResponseParser {
public:
    // 智能指针
    typedef std::shared_ptr<HttpResponseParser> ptr;

    /**
     * @brief http 响应解析器的构造函数
     *
     */
    HttpResponseParser();

public:
    /**
     * @brief 解析 http 响应
     *
     * @param data
     * @param len
     * @param chunck
     * @return size_t
     */
    size_t execute(char *data, size_t len, bool chunck);

    /**
     * @brief 是否解析完成
     *
     * @return int
     */
    int isFinished();

    /**
     * @brief 是否有错误
     *
     * @return int
     */
    int hasError();

    /**
     * @brief 获取 content-length
     *
     * @return uint64_t
     */
    uint64_t getContentLength();

    /**
     * @brief 获取解析的 http 响应对象
     *
     * @return HttpResponse::ptr
     */
    HttpResponse::ptr getData() const { return m_data; }

    /**
     * @brief 获取 http 解析器
     *
     * @return const httpclient_parser&
     */
    const httpclient_parser &getParser() const { return m_parser; }

    /**
     * @brief 设置错误
     *
     * @param v
     */
    void setError(int v) { m_error = v; }

public:
    static uint64_t GetHttpResponseBufferSize();

    static uint64_t GetHttpResponseMaxBodySize();

private:
    // http 响应解析器
    httpclient_parser m_parser;

    // http 响应对象
    HttpResponse::ptr m_data;

    // 1001: invalid version
    // 1002: invalid field
    int m_error;
};

}  // namespace http

}  // namespace ljrserver

#endif  //__LJRSERVER_HTTP_PARSER_H__
