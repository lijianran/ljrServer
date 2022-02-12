
#ifndef __LJRSERVER_HTTP_SERVLET_H__
#define __LSRSERVER_HTTP_SERVLET_H__

// 智能指针
#include <memory>
// 函数包装
#include <functional>
// 字符串
#include <string>
// 变长数组
#include <vector>
// 字典
#include <unordered_map>

// 封装 http 请求/响应
#include "http.h"
// http session 服务端封装
#include "http_session.h"
// 线程
#include "../thread.h"

namespace ljrserver {

namespace http {

/**
 * @brief Class 封装 Servlet 抽象类
 *
 */
class Servlet {
public:
    // 智能指针
    typedef std::shared_ptr<Servlet> ptr;

    /**
     * @brief Servlet 构造函数
     *
     * @param name 名称
     */
    Servlet(const std::string &name) : m_name(name) {}

    /**
     * @brief Servlet 析构函数 基类需要设置为虚函数
     *
     */
    virtual ~Servlet() {}

    /**
     * @brief Servlet 处理函数 纯虚函数
     *
     * @param request 请求
     * @param response 响应
     * @param session 服务端
     * @return int32_t
     */
    virtual int32_t handle(ljrserver::http::HttpRequest::ptr request,
                           ljrserver::http::HttpResponse::ptr response,
                           ljrserver::http::HttpSession::ptr session) = 0;

    /**
     * @brief 获取 Servlet 名称
     *
     * @return const std::string&
     */
    const std::string &getName() const { return m_name; }

protected:
    /// Servlet 名称
    std::string m_name;
};

/**
 * @brief Class 封装函数 Servlet
 *
 * 继承自 Servlet 抽象类
 */
class FunctionServlet : public Servlet {
public:
    // 智能指针
    typedef std::shared_ptr<FunctionServlet> ptr;

    // handle 函数包装
    typedef std::function<int32_t(ljrserver::http::HttpRequest::ptr request,
                                  ljrserver::http::HttpResponse::ptr response,
                                  ljrserver::http::HttpSession::ptr session)>
        callback;

    /**
     * @brief 函数 Servlet 构造函数
     *
     * @param cb 处理函数
     */
    FunctionServlet(callback cb);

    /**
     * @brief 纯虚函数的实现 Servlet 处理函数
     *
     * @param request 请求
     * @param response 响应
     * @param session 服务端
     * @return int32_t
     */
    int32_t handle(ljrserver::http::HttpRequest::ptr request,
                   ljrserver::http::HttpResponse::ptr response,
                   ljrserver::http::HttpSession::ptr session) override;

private:
    /// 处理函数
    callback m_cb;
};

/**
 * @brief Class 封装 Servlet 调遣器
 *
 * 继承自 Servlet 抽象类
 */
class ServletDispatch : public Servlet {
public:
    // 智能指针
    typedef std::shared_ptr<ServletDispatch> ptr;

    // 读写锁
    typedef RWMutex RWMutexType;

    /**
     * @brief Servlet 调遣器的构造函数
     *
     */
    ServletDispatch();

    /**
     * @brief 纯虚函数的实现 Servlet 处理函数
     *
     * @param request 请求
     * @param response 响应
     * @param session 服务端
     * @return int32_t
     */
    int32_t handle(ljrserver::http::HttpRequest::ptr request,
                   ljrserver::http::HttpResponse::ptr response,
                   ljrserver::http::HttpSession::ptr session) override;

public:  /// 新增
    void addServlet(const std::string &uri, Servlet::ptr slt);

    void addServlet(const std::string &uri, FunctionServlet::callback cb);

    void addGlobServlet(const std::string &uri, Servlet::ptr slt);

    void addGlobServlet(const std::string &uri, FunctionServlet::callback cb);

public:  /// 删除
    void delServlet(const std::string &uri);
    void delGlobServlet(const std::string &uri);

public:  /// 获取
    Servlet::ptr getServlet(const std::string &uri);
    Servlet::ptr getGlobServlet(const std::string &uri);

    Servlet::ptr getMatchedServlet(const std::string &uri);

public:  /// 默认
    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }

private:
    // 读写锁
    RWMutexType m_mutex;

    // URI (/ljrserver/xxx) -> servlet 精准命中 字典
    std::unordered_map<std::string, Servlet::ptr> m_datas;

    // URI (/ljrserver/*  ) -> servlet 模糊命中 变长数组
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;

    // 默认的 servlet 所有路径都没有匹配到时使用 404
    Servlet::ptr m_default;
};

/**
 * @brief Class 封装空 Servlet
 *
 * 继承自 Servlet 抽象类
 */
class NotFoundServlet : public Servlet {
public:
    // 智能指针
    typedef std::shared_ptr<NotFoundServlet> ptr;

    /**
     * @brief 空 Servlet 构造函数
     *
     */
    NotFoundServlet();

    /**
     * @brief 纯虚函数的实现 Servlet 处理函数
     *
     * 不实现则仍是抽象类 无法实例化
     *
     * @param request 请求
     * @param response 响应
     * @param session 服务端
     * @return int32_t
     */
    int32_t handle(ljrserver::http::HttpRequest::ptr request,
                   ljrserver::http::HttpResponse::ptr response,
                   ljrserver::http::HttpSession::ptr session) override;
};

}  // namespace http

}  // namespace ljrserver

#endif  //__LJRSERVER_HTTP_SERVLET_H__