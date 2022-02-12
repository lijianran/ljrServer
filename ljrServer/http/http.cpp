
#include "http.h"

namespace ljrserver {

namespace http {

/***************
辅助方法
***************/
/**
 * @brief string 字符串转 http 请求方法
 *
 * @param m
 * @return HttpMethod
 */
HttpMethod StringToHttpMethod(const std::string &m) {
#define XX(num, name, string)                  \
    {                                          \
        if (strcmp(#string, m.c_str()) == 0) { \
            return HttpMethod::name;           \
        }                                      \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

/**
 * @brief char* 字符串转 http 请求方法
 *
 * @param m
 * @return HttpMethod
 */
HttpMethod CharsToHttpMethod(const char *m) {
#define XX(num, name, string)                            \
    {                                                    \
        if (strncmp(#string, m, strlen(#string)) == 0) { \
            return HttpMethod::name;                     \
        }                                                \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

// 静态 http 请求方法数组
static const char *s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

/**
 * @brief http 请求方法转字符串
 *
 * @param m
 * @return const char*
 */
const char *HttpMethodToString(const HttpMethod &m) {
    uint32_t index = (uint32_t)m;
    if (index > sizeof(s_method_string) / sizeof(s_method_string[0])) {
        return "<unknown>";
    }
    return s_method_string[index];
}

/**
 * @brief http 状态转字符串
 *
 * @param s
 * @return const char*
 */
const char *HttpStatusToString(const HttpStatus &s) {
    switch (s) {
#define XX(code, name, msg) \
    case HttpStatus::name:  \
        return #msg;

        HTTP_STATUS_MAP(XX);
#undef XX
        default:
            return "<unknown>";
    }
}

/**
 * @brief 仿函数 重载类/结构体的 () 操作符
 *
 * @param lhs
 * @param rhs
 * @return true
 * @return false
 */
bool CaseInsensitiveLess::operator()(const std::string &lhs,
                                     const std::string &rhs) const {
    // 不分大小写比较
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

/***************
HttpRequest
***************/
/**
 * @brief http 请求对象构造函数
 *
 * @param version 版本 [= 0x11]
 * @param close 是否长连接 [= true] 默认非长连接
 */
HttpRequest::HttpRequest(uint8_t version, bool close)
    : m_method(HttpMethod::GET),
      m_version(version),
      m_close(close),
      m_path("/") {
    // 初始化列表顺序与成员变量定义顺序一致
    // 默认 GET method http1.1 长连接 根目录
}

/// GET 获取

std::string HttpRequest::getHeader(const std::string &key,
                                   const std::string &def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

std::string HttpRequest::getParam(const std::string &key,
                                  const std::string &def) const {
    auto it = m_params.find(key);
    return it == m_params.end() ? def : it->second;
}

std::string HttpRequest::getCookie(const std::string &key,
                                   const std::string &def) const {
    auto it = m_cookies.find(key);
    return it == m_cookies.end() ? def : it->second;
}

/// SET 设置

void HttpRequest::setHeader(const std::string &key, const std::string &val) {
    m_headers[key] = val;
}

void HttpRequest::setParam(const std::string &key, const std::string &val) {
    m_params[key] = val;
}

void HttpRequest::setCookie(const std::string &key, const std::string &val) {
    m_cookies[key] = val;
}

/// DEL 删除

void HttpRequest::delHeader(const std::string &key) { m_headers.erase(key); }

void HttpRequest::delParam(const std::string &key) { m_params.erase(key); }

void HttpRequest::delCookie(const std::string &key) { m_cookies.erase(key); }

/// HAS 包含

bool HttpRequest::hasHeader(const std::string &key, std::string *val) {
    auto it = m_headers.find(key);
    if (it == m_headers.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasParam(const std::string &key, std::string *val) {
    auto it = m_params.find(key);
    if (it == m_params.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasCookie(const std::string &key, std::string *val) {
    auto it = m_cookies.find(key);
    if (it == m_cookies.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

/// 打印输出

std::ostream &HttpRequest::dump(std::ostream &os) const {
    /**
     * GET /uri HTTP/1.1
     * host: www.baidu.com
     *
     * m_body
     */
    os << HttpMethodToString(m_method) << " " << m_path
       << (m_query.empty() ? "" : "?") << m_query
       << (m_fragment.empty() ? "" : "#") << m_fragment << " HTTP/"
       << ((uint32_t)m_version >> 4) << "." << ((uint32_t)(m_version & 0x0F))
       << "\r\n";

    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

    for (auto &i : m_headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0)
            continue;
        os << i.first << ":" << i.second << "\r\n";
    }

    if (!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n" << m_body;
    } else {
        os << "\r\n";
    }

    return os;
}

std::string HttpRequest::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

/***************
HttpResponse
***************/
/**
 * @brief http 响应对象构造函数
 *
 * @param version 版本 [= 0x11]
 * @param close 是否长连接 [= true] 默认非长连接
 */
HttpResponse::HttpResponse(uint8_t version, bool close)
    : m_status(HttpStatus::OK), m_version(version), m_close(close) {
    // 初始化列表顺序与成员变量定义顺序一致
    // 默认 200 OK http1.1 长连接
}

/// 响应 Header

std::string HttpResponse::getHeader(const std::string &key,
                                    std::string &def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpResponse::setHeader(const std::string &key, const std::string &val) {
    m_headers[key] = val;
}

void HttpResponse::delHeader(const std::string &key) { m_headers.erase(key); }

/// 打印输出

std::ostream &HttpResponse::dump(std::ostream &os) const {
    os << "HTTP/" << ((uint32_t)(m_version >> 4)) << "."
       << ((uint32_t)(m_version & 0x0F)) << " " << (uint32_t)m_status << " "
       << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
       << "\r\n";

    for (auto &i : m_headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0)
            continue;
        os << i.first << ": " << i.second << "\r\n";
    }
    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

    if (!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n" << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::string HttpResponse::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

/***************
流式输出 对应 toString()
***************/

/**
 * @brief 重载 http 请求类的流式操作符
 *
 * @param os
 * @param req
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const HttpRequest &req) {
    return req.dump(os);
}

/**
 * @brief 重载 http 响应类的流式操作符
 *
 * @param os
 * @param rsp
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const HttpResponse &rsp) {
    return rsp.dump(os);
}

}  // namespace http

}  // namespace ljrserver
