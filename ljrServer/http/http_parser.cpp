
#include "http_parser.h"

// 日志
#include "../log.h"
// 配置
#include "../config.h"

// 字符串
#include <string.h>

namespace ljrserver {

namespace http {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

// 配置 http 请求缓存大小 4m
static ljrserver::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    ljrserver::Config::Lookup("http.request.buffer_size", (uint64_t)(4 * 1024),
                              "http request buffer size");

// 配置 http 请求体大小
static ljrserver::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    ljrserver::Config::Lookup("http.request.max_body_size",
                              (uint64_t)(64 * 1024 * 1024),
                              "http request max body size");

// 配置 http 响应缓存大小
static ljrserver::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
    ljrserver::Config::Lookup("http.response.buffer_size", (uint64_t)(4 * 1024),
                              "http response buffer size");

// 配置 http 响应体大小
static ljrserver::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
    ljrserver::Config::Lookup("http.response.max_body_size",
                              (uint64_t)(64 * 1024 * 1024),
                              "http response max body size");

/// 全局静态变量 只能本文件使用

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;

/// 供外部调用

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
    return s_http_request_max_body_size;
}

uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseMaxBodySize() {
    return s_http_response_max_body_size;
}

/**
 * @brief 匿名命名空间
 *
 */
namespace {
/**
 * @brief 初始化 http 相关配置
 *
 */
struct _RequestSizeIniter {
    /**
     * @brief 构造函数 初始化配置
     *
     */
    _RequestSizeIniter() {
        // 获取约定的配置值
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size =
            g_http_response_max_body_size->getValue();

        // 添加配置变更监听事件 及时更新配置
        g_http_request_buffer_size->addListener(
            [](const uint64_t &old_value, const uint64_t &new_value) {
                s_http_request_buffer_size = new_value;
            });

        g_http_request_max_body_size->addListener(
            [](const uint64_t &old_value, const uint64_t &new_value) {
                s_http_request_max_body_size = new_value;
            });

        g_http_response_buffer_size->addListener(
            [](const uint64_t &old_value, const uint64_t &new_value) {
                s_http_response_buffer_size = new_value;
            });

        g_http_response_max_body_size->addListener(
            [](const uint64_t &old_value, const uint64_t &new_value) {
                s_http_response_max_body_size = new_value;
            });
    }
};

// 全局静态变量的构造函数在 main 函数之前执行
static _RequestSizeIniter _init;

}  // namespace

/******************************
HttpRequestParser
******************************/

void on_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    HttpMethod m = CharsToHttpMethod(at);

    if (m == HttpMethod::INVALID_METHOD) {
        LJRSERVER_LOG_WARN(g_logger)
            << "invalid http request method: " << std::string(at, length);
        parser->setError(1000);
        return;
    }

    parser->getData()->setMethod(m);
}

void on_request_uri(void *data, const char *at, size_t length) {}

void on_request_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    parser->getData()->setFragment(std::string(at, length));
}

void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    parser->getData()->setPath(std::string(at, length));
}

void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    parser->getData()->setQuery(std::string(at, length));
}

void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    uint8_t v = 0;
    if (strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if (strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        LJRSERVER_LOG_WARN(g_logger)
            << "invalid http request version: " << std::string(at, length);
        parser->setError(1001);
        return;
    }

    parser->getData()->setVersion(v);
}

void on_request_header_done(void *data, const char *at, size_t length) {}

void on_request_http_field(void *data, const char *field, size_t flen,
                           const char *value, size_t vlen) {
    HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
    if (flen == 0) {
        LJRSERVER_LOG_WARN(g_logger) << "invalid http request field length = 0";
        // parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen),
                                 std::string(value, vlen));
}

/**
 * @brief http 请求解析器的构造函数
 *
 */
HttpRequestParser::HttpRequestParser() : m_error(0) {
    // http 请求解析结果对象
    m_data.reset(new ljrserver::http::HttpRequest);

    // 配置 http 请求解析器
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;

    m_parser.http_field = on_request_http_field;

    m_parser.data = this;
}

/**
 * @brief 解析 http 请求
 *
 * @param data
 * @param len
 * @return size_t
 */
size_t HttpRequestParser::execute(char *data, size_t len) {
    // 1  成功
    // -1 有错误
    // >0 已经处理的字节数，且data有效数据为 len - v
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    // if (offset == -1)
    // {
    //     LJRSERVER_LOG_WARN(g_logger) << "invalid request:" <<
    //     std::string(buff, len);
    // }
    // else if (offset != -1)
    // {
    //     memmove(data, data + offset, len - offset);
    // }
    memmove(data, data + offset, (len - offset));
    return offset;
}

/**
 * @brief 是否解析完成
 *
 * @return int
 */
int HttpRequestParser::isFinished() {
    // 是否完成
    return http_parser_finish(&m_parser);
}

/**
 * @brief 是否有错误
 *
 * @return int
 */
int HttpRequestParser::hasError() {
    // 获取错误
    return m_error || http_parser_has_error(&m_parser);
}

/**
 * @brief 获取 content-length
 *
 * @return uint64_t
 */
uint64_t HttpRequestParser::getContentLength() {
    // 请求体内容长度
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

/******************************
HttpResponseParser
******************************/

void on_response_reason(void *data, const char *at, size_t length) {
    HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
    parser->getData()->setReason(std::string(at, length));
}

void on_response_status(void *data, const char *at, size_t length) {
    HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
    HttpStatus status = (HttpStatus)(atoi(at));

    parser->getData()->setStatus(status);
}

void on_response_chunk(void *data, const char *at, size_t length) {}

void on_response_version(void *data, const char *at, size_t length) {
    HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
    uint8_t v = 0;
    if (strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if (strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        LJRSERVER_LOG_WARN(g_logger)
            << "invalid http response version: " << std::string(at, length);
        parser->setError(1001);
        return;
    }

    parser->getData()->setVersion(v);
}

void on_response_header_down(void *data, const char *at, size_t length) {}

void on_response_last_chunk(void *data, const char *at, size_t length) {}

void on_response_http_field(void *data, const char *field, size_t flen,
                            const char *value, size_t vlen) {
    HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
    if (flen == 0) {
        LJRSERVER_LOG_WARN(g_logger)
            << "invalid http response field length = 0";
        // parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen),
                                 std::string(value, vlen));
}

/**
 * @brief http 响应解析器的构造函数
 *
 */
HttpResponseParser::HttpResponseParser() : m_error(0) {
    // http 响应解析结果对象
    m_data.reset(new ljrserver::http::HttpResponse);

    // 配置 http 响应解析器
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_down;
    m_parser.last_chunk = on_response_last_chunk;

    m_parser.http_field = on_response_http_field;

    m_parser.data = this;
}

/**
 * @brief 解析 http 响应
 *
 * @param data
 * @param len
 * @param chunck
 * @return size_t
 */
size_t HttpResponseParser::execute(char *data, size_t len, bool chunck) {
    if (chunck) {
        httpclient_parser_init(&m_parser);
    }
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, len - offset);
    return offset;
}

/**
 * @brief 是否解析完成
 *
 * @return int
 */
int HttpResponseParser::isFinished() {
    return httpclient_parser_finish(&m_parser);
}

/**
 * @brief 是否有错误
 *
 * @return int
 */
int HttpResponseParser::hasError() {
    return m_error || httpclient_parser_has_error(&m_parser);
}

/**
 * @brief 获取 content-length
 *
 * @return uint64_t
 */
uint64_t HttpResponseParser::getContentLength() {
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

}  // namespace http

}  // namespace ljrserver
