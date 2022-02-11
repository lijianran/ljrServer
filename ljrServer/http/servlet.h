
#ifndef __LJRSERVER_HTTP_SERVLET_H__
#define __LSRSERVER_HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include "http.h"
#include "http_session.h"
#include "../thread.h"

namespace ljrserver {

namespace http {

class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string &name) : m_name(name) {}

    virtual ~Servlet() {}

    virtual int32_t handle(ljrserver::http::HttpRequest::ptr request,
                           ljrserver::http::HttpResponse::ptr response,
                           ljrserver::http::HttpSession::ptr session) = 0;

    const std::string &getName() const { return m_name; }

protected:
    std::string m_name;
};

class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;

    typedef std::function<int32_t(ljrserver::http::HttpRequest::ptr request,
                                  ljrserver::http::HttpResponse::ptr response,
                                  ljrserver::http::HttpSession::ptr session)>
        callback;

    FunctionServlet(callback cb);

    int32_t handle(ljrserver::http::HttpRequest::ptr request,
                   ljrserver::http::HttpResponse::ptr response,
                   ljrserver::http::HttpSession::ptr session) override;

private:
    callback m_cb;
};

class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType;

    ServletDispatch();

    int32_t handle(ljrserver::http::HttpRequest::ptr request,
                   ljrserver::http::HttpResponse::ptr response,
                   ljrserver::http::HttpSession::ptr session) override;

    void addServlet(const std::string &uri, Servlet::ptr slt);

    void addServlet(const std::string &uri, FunctionServlet::callback cb);

    void addGlobServlet(const std::string &uri, Servlet::ptr slt);

    void addGlobServlet(const std::string &uri, FunctionServlet::callback cb);

    void delServlet(const std::string &uri);
    void delGlobServlet(const std::string &uri);

    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }

    Servlet::ptr getServlet(const std::string &uri);
    Servlet::ptr getGlobServlet(const std::string &uri);

    Servlet::ptr getMatchedServlet(const std::string &uri);

private:
    RWMutexType m_mutex;
    // uri (/ljrserver/xxx) -> servlet 精准命中
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    // uri (/ljrserver/*) -> servlet 模糊命中
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
    // 默认servlet，所有路径都没有匹配到时使用
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet();

    int32_t handle(ljrserver::http::HttpRequest::ptr request,
                   ljrserver::http::HttpResponse::ptr response,
                   ljrserver::http::HttpSession::ptr session) override;
};

}  // namespace http

}  // namespace ljrserver

#endif  //__LJRSERVER_HTTP_SERVLET_H__