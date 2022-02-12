
#include "servlet.h"
// 模糊匹配
#include <fnmatch.h>

namespace ljrserver {

namespace http {

/*********************************************
 * FunctionServlet 自定义处理函数
 *********************************************/
/**
 * @brief 函数 Servlet 构造函数
 *
 * @param cb 处理函数
 */
FunctionServlet::FunctionServlet(callback cb)
    : Servlet("FunctionServlet"), m_cb(cb) {
    // 初始化 Servlet 抽象类
}

/**
 * @brief 纯虚函数的实现 Servlet 处理函数
 *
 * @param request 请求
 * @param response 响应
 * @param session 服务端
 * @return int32_t
 */
int32_t FunctionServlet::handle(ljrserver::http::HttpRequest::ptr request,
                                ljrserver::http::HttpResponse::ptr response,
                                ljrserver::http::HttpSession::ptr session) {
    // 执行处理函数
    return m_cb(request, response, session);
}

/*********************************************
 * ServletDispatch 分派
 *********************************************/
/**
 * @brief Servlet 调遣器的构造函数
 *
 */
ServletDispatch::ServletDispatch() : Servlet("ServletDispatch") {
    // 默认的 servlet 所有路径都没有匹配到时使用 NotFoundServlet
    m_default.reset(new NotFoundServlet());
}

/**
 * @brief 纯虚函数的实现 Servlet 处理函数
 *
 * @param request 请求
 * @param response 响应
 * @param session 服务端
 * @return int32_t
 */
int32_t ServletDispatch::handle(ljrserver::http::HttpRequest::ptr request,
                                ljrserver::http::HttpResponse::ptr response,
                                ljrserver::http::HttpSession::ptr session) {
    // 根据 request 请求的 path 路径来获取相匹配的 Servlet
    auto slt = getMatchedServlet(request->getPath());
    if (slt) {
        // 命中了 Servlet
        // 基类指针 动态多态 执行处理函数
        slt->handle(request, response, session);
    }
    return 0;
}

/// Add 新增
/**
 * @brief 增加精准命中的 Servlet 到字典
 *
 * @param uri
 * @param slt
 */
void ServletDispatch::addServlet(const std::string &uri, Servlet::ptr slt) {
    // 上写锁
    RWMutexType::WriteLock lock(m_mutex);
    // 存入精准命中的 Servlet 字典
    m_datas[uri] = slt;
}

/**
 * @brief 增加精准命中的 FunctionServlet 到字典
 *
 * @param uri
 * @param cb
 */
void ServletDispatch::addServlet(const std::string &uri,
                                 FunctionServlet::callback cb) {
    // 上写锁
    RWMutexType::WriteLock lock(m_mutex);
    // 包装成 FunctionServlet 加入精准命中的 Servlet 字典
    m_datas[uri].reset(new FunctionServlet(cb));
}

/**
 * @brief 增加模糊命中的 Servlet 到变长数组
 *
 * @param uri
 * @param slt
 */
void ServletDispatch::addGlobServlet(const std::string &uri, Servlet::ptr slt) {
    // 上写锁
    RWMutexType::WriteLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        // 先从变长数组中删除
        if (it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    // 再加入数组尾部
    m_globs.push_back(std::make_pair(uri, slt));
}

/**
 * @brief 增加模糊命中的 FunctionServlet 到变长数组
 *
 * @param uri
 * @param cb
 */
void ServletDispatch::addGlobServlet(const std::string &uri,
                                     FunctionServlet::callback cb) {
    // 包装成 FunctionServlet
    return addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
}

/// Del 删除
/**
 * @brief 删除精准命中的 Servlet
 *
 * @param uri
 */
void ServletDispatch::delServlet(const std::string &uri) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas.erase(uri);
}

/**
 * @brief 删除模糊命中的 Servlet
 *
 * @param uri
 */
void ServletDispatch::delGlobServlet(const std::string &uri) {
    RWMutexType::WriteLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if (it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
}

/// Get 获取
/**
 * @brief 获取精准命中的 Servlet
 *
 * @param uri
 * @return Servlet::ptr
 */
Servlet::ptr ServletDispatch::getServlet(const std::string &uri) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}

/**
 * @brief 获取模糊命中的 Servlet
 *
 * @param uri
 * @return Servlet::ptr
 */
Servlet::ptr ServletDispatch::getGlobServlet(const std::string &uri) {
    RWMutexType::ReadLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if (it->first == uri) {
            return it->second;
        }
    }
    return nullptr;
}

/**
 * @brief 获取匹配的 Servlet
 *
 * @param uri
 * @return Servlet::ptr
 */
Servlet::ptr ServletDispatch::getMatchedServlet(const std::string &uri) {
    // 上读锁
    RWMutexType::ReadLock lock(m_mutex);
    // 先匹配精准路径的 Servlet
    auto mit = m_datas.find(uri);
    if (mit != m_datas.end()) {
        // 命中 返回 Servlet
        return mit->second;
    }

    // 再匹配模糊路径的 Servlet
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        // if (it->first == uri)
        // {
        //     return it->second;
        // }
        // fnmatch 模糊匹配
        if (!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            // 命中 返回 Servlet
            return it->second;
        }
    }

    // 路径命中失败 返回 NotFoundServlet 404 资源
    return m_default;
}

/*********************************************
 * NotFoundServlet 404
 *********************************************/
/**
 * @brief 空 Servlet 构造函数
 *
 */
NotFoundServlet::NotFoundServlet() : Servlet("NotFoundServlet") {
    // m_content = "<html><head><title>404 Not Found"
    //             "</title></head><body><center><h1>404 Not
    //             Found</h1></center>"
    //             "<hr><center>" +
    //             name + "</center></body></html>";
}

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
int32_t NotFoundServlet::handle(ljrserver::http::HttpRequest::ptr request,
                                ljrserver::http::HttpResponse::ptr response,
                                ljrserver::http::HttpSession::ptr session) {
    // 404 响应内容
    static const std::string &RSP_BODY =
        "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>ljrserver/1.0.0</center></body></html>";

    // 设置 http 状态
    response->setStatus(ljrserver::http::HttpStatus::NOT_FOUND);
    // 设置服务器信息
    response->setHeader("Server", "ljrServer/1.0.0");
    // 设置内容格式
    response->setHeader("Content-Type", "text/html");
    // 设置响应体
    response->setBody(RSP_BODY);

    return 0;
}

}  // namespace http

}  // namespace ljrserver
