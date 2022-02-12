
#ifndef __LJRSERVER_HTTP_HTTP_H__
#define __LJRSERVER_HTTP_HTTP_H__

#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <boost/lexical_cast.hpp>

namespace ljrserver {

namespace http {

/* Request Methods */
#define HTTP_METHOD_MAP(XX)          \
    XX(0, DELETE, DELETE)            \
    XX(1, GET, GET)                  \
    XX(2, HEAD, HEAD)                \
    XX(3, POST, POST)                \
    XX(4, PUT, PUT)                  \
    /* pathological */               \
    XX(5, CONNECT, CONNECT)          \
    XX(6, OPTIONS, OPTIONS)          \
    XX(7, TRACE, TRACE)              \
    /* WebDAV */                     \
    XX(8, COPY, COPY)                \
    XX(9, LOCK, LOCK)                \
    XX(10, MKCOL, MKCOL)             \
    XX(11, MOVE, MOVE)               \
    XX(12, PROPFIND, PROPFIND)       \
    XX(13, PROPPATCH, PROPPATCH)     \
    XX(14, SEARCH, SEARCH)           \
    XX(15, UNLOCK, UNLOCK)           \
    XX(16, BIND, BIND)               \
    XX(17, REBIND, REBIND)           \
    XX(18, UNBIND, UNBIND)           \
    XX(19, ACL, ACL)                 \
    /* subversion */                 \
    XX(20, REPORT, REPORT)           \
    XX(21, MKACTIVITY, MKACTIVITY)   \
    XX(22, CHECKOUT, CHECKOUT)       \
    XX(23, MERGE, MERGE)             \
    /* upnp */                       \
    XX(24, MSEARCH, M - SEARCH)      \
    XX(25, NOTIFY, NOTIFY)           \
    XX(26, SUBSCRIBE, SUBSCRIBE)     \
    XX(27, UNSUBSCRIBE, UNSUBSCRIBE) \
    /* RFC-5789 */                   \
    XX(28, PATCH, PATCH)             \
    XX(29, PURGE, PURGE)             \
    /* CalDAV */                     \
    XX(30, MKCALENDAR, MKCALENDAR)   \
    /* RFC-2068, section 19.6.1.2 */ \
    XX(31, LINK, LINK)               \
    XX(32, UNLINK, UNLINK)           \
    /* icecast */                    \
    XX(33, SOURCE, SOURCE)

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                   \
    XX(100, CONTINUE, Continue)                                               \
    XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                         \
    XX(102, PROCESSING, Processing)                                           \
    XX(200, OK, OK)                                                           \
    XX(201, CREATED, Created)                                                 \
    XX(202, ACCEPTED, Accepted)                                               \
    XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)   \
    XX(204, NO_CONTENT, No Content)                                           \
    XX(205, RESET_CONTENT, Reset Content)                                     \
    XX(206, PARTIAL_CONTENT, Partial Content)                                 \
    XX(207, MULTI_STATUS, Multi - Status)                                     \
    XX(208, ALREADY_REPORTED, Already Reported)                               \
    XX(226, IM_USED, IM Used)                                                 \
    XX(300, MULTIPLE_CHOICES, Multiple Choices)                               \
    XX(301, MOVED_PERMANENTLY, Moved Permanently)                             \
    XX(302, FOUND, Found)                                                     \
    XX(303, SEE_OTHER, See Other)                                             \
    XX(304, NOT_MODIFIED, Not Modified)                                       \
    XX(305, USE_PROXY, Use Proxy)                                             \
    XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                           \
    XX(308, PERMANENT_REDIRECT, Permanent Redirect)                           \
    XX(400, BAD_REQUEST, Bad Request)                                         \
    XX(401, UNAUTHORIZED, Unauthorized)                                       \
    XX(402, PAYMENT_REQUIRED, Payment Required)                               \
    XX(403, FORBIDDEN, Forbidden)                                             \
    XX(404, NOT_FOUND, Not Found)                                             \
    XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                           \
    XX(406, NOT_ACCEPTABLE, Not Acceptable)                                   \
    XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)     \
    XX(408, REQUEST_TIMEOUT, Request Timeout)                                 \
    XX(409, CONFLICT, Conflict)                                               \
    XX(410, GONE, Gone)                                                       \
    XX(411, LENGTH_REQUIRED, Length Required)                                 \
    XX(412, PRECONDITION_FAILED, Precondition Failed)                         \
    XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                             \
    XX(414, URI_TOO_LONG, URI Too Long)                                       \
    XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                   \
    XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                     \
    XX(417, EXPECTATION_FAILED, Expectation Failed)                           \
    XX(421, MISDIRECTED_REQUEST, Misdirected Request)                         \
    XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                       \
    XX(423, LOCKED, Locked)                                                   \
    XX(424, FAILED_DEPENDENCY, Failed Dependency)                             \
    XX(426, UPGRADE_REQUIRED, Upgrade Required)                               \
    XX(428, PRECONDITION_REQUIRED, Precondition Required)                     \
    XX(429, TOO_MANY_REQUESTS, Too Many Requests)                             \
    XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
    XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)     \
    XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                     \
    XX(501, NOT_IMPLEMENTED, Not Implemented)                                 \
    XX(502, BAD_GATEWAY, Bad Gateway)                                         \
    XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                         \
    XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                 \
    XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)           \
    XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                 \
    XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                       \
    XX(508, LOOP_DETECTED, Loop Detected)                                     \
    XX(510, NOT_EXTENDED, Not Extended)                                       \
    XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

/**
 * @brief 枚举类
 *
 * http 请求方法 get post ...
 */
enum class HttpMethod {
#define XX(num, name, string) name = num,
    /* Request Methods */
    HTTP_METHOD_MAP(XX)
#undef XX
    // 非法 method
    INVALID_METHOD
};

/**
 * @brief 枚举类
 *
 * http 状态 200 ok ...
 */
enum class HttpStatus {
#define XX(code, name, desc) name = code,
    /* Status Codes */
    HTTP_STATUS_MAP(XX)
#undef XX
};

/**
 * @brief string 字符串转 http 请求方法
 *
 * @param m
 * @return HttpMethod
 */
HttpMethod StringToHttpMethod(const std::string &m);

/**
 * @brief char* 字符串转 http 请求方法
 *
 * @param m
 * @return HttpMethod
 */
HttpMethod CharsToHttpMethod(const char *m);

/**
 * @brief http 请求方法转字符串
 *
 * @param m
 * @return const char*
 */
const char *HttpMethodToString(const HttpMethod &m);

/**
 * @brief http 状态转字符串
 *
 * @param s
 * @return const char*
 */
const char *HttpStatusToString(const HttpStatus &s);

/**
 * @brief 实现 map 比较仿函数，大小写无关
 *
 */
struct CaseInsensitiveLess {
    /**
     * @brief 仿函数 重载类/结构体的 () 操作符
     *
     * @param lhs
     * @param rhs
     * @return true
     * @return false
     */
    bool operator()(const std::string &lhs, const std::string &rhs) const;
};

/**
 * @brief 模版类 检查并动态转换 map 中某个键的值为 T 类型
 *
 * @tparam MapType
 * @tparam T
 * @param m map
 * @param key key
 * @param val 返回值
 * @param def 默认值
 * @return true
 * @return false
 */
template <class MapType, class T>
bool checkGetAs(const MapType &m, const std::string &key, T &val,
                const T &def = T()) {
    // std::string std;
    auto it = m.find(key);
    if (it == m.end()) {
        // 没找到 使用默认值
        val = def;
        return false;
    }

    try {
        // 动态转换
        val = boost::lexical_cast<T>(it->second);
        return true;
    } catch (const std::exception &e) {
        // 抛出异常 使用默认值
        val = def;
        // std::cerr << e.what() << '\n';
    }
    return false;
}

/**
 * @brief 模版类 动态转换 map 中某个键的值为 T 类型
 *
 * @tparam MapType
 * @tparam T
 * @param m map
 * @param key key
 * @param def 默认值
 * @return T
 */
template <class MapType, class T>
T getAs(const MapType &m, const std::string &key, const T &def = T()) {
    // 在 map 中查找 key
    auto it = m.find(key);
    if (it == m.end()) {
        // 没找到 直接返回默认值
        return def;
    }

    try {
        // 直接转换
        return boost::lexical_cast<T>(it->second);
    } catch (const std::exception &e) {
        // 抛出异常
        std::cerr << e.what() << '\n';
    }
    // 转换失败 返回默认值
    return def;
}

/**
 * @brief Class 封装 http 请求
 *
 */
class HttpRequest {
public:
    // 智能指针
    typedef std::shared_ptr<HttpRequest> ptr;

    // map 类型
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief http 请求对象构造函数
     *
     * @param version 版本 [= 0x11]
     * @param close 是否长连接 [= true] 默认非长连接
     */
    HttpRequest(uint8_t version = 0x11, bool close = true);

public:
    HttpMethod getMethod() const { return m_method; }
    // HttpStatus getStatus() const { return m_status; }
    uint8_t getVersion() const { return m_version; }
    const std::string &getPath() const { return m_path; }
    const std::string &getQuery() const { return m_query; }
    const std::string &getFragment() const { return m_fragment; }
    const std::string &getBody() const { return m_body; }
    const MapType &getHeaders() const { return m_headers; }
    const MapType &getParams() const { return m_params; }
    const MapType &getCookies() const { return m_cookies; }

    void setMethod(HttpMethod m) { m_method = m; }
    // void setStatus(HttpStatus s) { m_status = s; }
    void setVersion(uint8_t v) { m_version = v; }
    void setPath(const std::string &v) { m_path = v; }
    void setQuery(const std::string &v) { m_query = v; }
    void setFragment(const std::string &v) { m_fragment = v; }
    void setBody(const std::string &v) { m_body = v; }
    void setHeaders(const MapType &v) { m_headers = v; }
    void setParams(const MapType &v) { m_params = v; }
    void setCookies(const MapType &v) { m_cookies = v; }

    bool isCose() const { return m_close; }
    void setClose(bool v) { m_close = v; }

public:
    std::string getHeader(const std::string &key,
                          const std::string &def = "") const;
    std::string getParam(const std::string &key,
                         const std::string &def = "") const;
    std::string getCookie(const std::string &key,
                          const std::string &def = "") const;

    void setHeader(const std::string &key, const std::string &val);
    void setParam(const std::string &key, const std::string &val);
    void setCookie(const std::string &key, const std::string &val);

    void delHeader(const std::string &key);
    void delParam(const std::string &key);
    void delCookie(const std::string &key);

    bool hasHeader(const std::string &key, std::string *val = nullptr);
    bool hasParam(const std::string &key, std::string *val = nullptr);
    bool hasCookie(const std::string &key, std::string *val = nullptr);

public:
    template <class T>
    bool checkGetHeaderAs(const std::string &key, T &val, const T &def = T()) {
        return checkGetAs(m_headers, key, val, def);
    }

    template <class T>
    T getHeaderAs(const std::string &key, const T &def = T()) {
        return getAs(m_headers, key, def);
    }

    template <class T>
    bool checkGetParamAs(const std::string &key, T &val, const T &def = T()) {
        return checkGetAs(m_params, key, val, def);
    }

    template <class T>
    T getParamAs(const std::string &key, const T &def = T()) {
        return getAs(m_params, key, def);
    }

    template <class T>
    bool checkGetCookieAs(const std::string &key, T &val, const T &def = T()) {
        return checkGetAs(m_cookies, key, val, def);
    }

    template <class T>
    T getCookieAs(const std::string &key, const T &def = T()) {
        return getAs(m_cookies, key, def);
    }

public:
    std::ostream &dump(std::ostream &os) const;

    std::string toString() const;

private:
    // 请求方法
    HttpMethod m_method;

    // 状态
    // HttpStatus m_status;

    // 版本
    uint8_t m_version;

    // 是否长连接
    bool m_close;

    // 请求 path
    std::string m_path;

    // 请求 query
    std::string m_query;

    // 请求 fragment
    std::string m_fragment;

    // 请求 body
    std::string m_body;

    MapType m_headers;
    MapType m_params;
    MapType m_cookies;
};

/**
 * @brief Class 封装 http 响应
 *
 */
class HttpResponse {
public:
    // 智能指针
    typedef std::shared_ptr<HttpResponse> ptr;

    // map 类型
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief http 响应对象构造函数
     *
     * @param version 版本 [= 0x11]
     * @param close 是否长连接 [= true] 默认非长连接
     */
    HttpResponse(uint8_t version = 0x11, bool close = true);

public:
    HttpStatus getStatus() const { return m_status; }
    uint8_t getVersion() const { return m_version; }
    const std::string &getBody() const { return m_body; }
    const std::string &getReason() const { return m_reason; }
    const MapType &getHeaders() const { return m_headers; }

    void setStatus(HttpStatus s) { m_status = s; }
    void setVersion(uint8_t v) { m_version = v; }
    void setBody(const std::string &b) { m_body = b; }
    void setReason(const std::string &r) { m_reason = r; }
    void setHeaders(const MapType &m) { m_headers = m; }

    bool isCose() const { return m_close; }
    void setClose(bool v) { m_close = v; }

public:
    std::string getHeader(const std::string &key, std::string &def) const;
    void setHeader(const std::string &key, const std::string &val);
    void delHeader(const std::string &key);

public:
    template <class T>
    bool checkGetHeaderAs(const std::string &key, T &val, const T &def = T()) {
        return checkGetAs(m_headers, key, val, def);
    }

    template <class T>
    T getHeaderAs(const std::string &key, const T &def = T()) {
        return getAs(m_headers, key, def);
    }

public:
    std::ostream &dump(std::ostream &os) const;

    std::string toString() const;

private:
    // http 状态
    HttpStatus m_status;

    // 版本
    uint8_t m_version;

    // 是否长连接
    bool m_close;

    // 响应 body
    std::string m_body;

    // 响应 reason
    std::string m_reason;

    // 响应 header
    MapType m_headers;
};

/**
 * @brief 重载 http 请求类的流式操作符
 *
 * @param os
 * @param req
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const HttpRequest &req);

/**
 * @brief 重载 http 响应类的流式操作符
 *
 * @param os
 * @param rsp
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const HttpResponse &rsp);

}  // namespace http

}  // namespace ljrserver

#endif  //__LJRSERVER_HTTP_HTTP_H__