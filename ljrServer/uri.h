
#ifndef __LJRSERVER_URI_H__
#define __LJRSERVER_URI_H__

#include <memory>
#include <string>

// 地址
#include "address.h"

namespace ljrserver {

/*
 foo://user@sylar.com:8042/over/there?name=ferret#nose
   \_/   \______________/\_________/ \_________/ \__/
    |           |            |            |        |
 scheme     authority       path        query   fragment
*/

/**
 * @brief Class 封装统一资源标志符 (Uniform Resource Identifier, URI)
 *
 */
class Uri {
public:
    // 智能指针
    typedef std::shared_ptr<Uri> ptr;

    /**
     * @brief 根据字符串创建 URI 对象
     *
     * @param uristr 字符串 std::string
     * @return Uri::ptr
     */
    static Uri::ptr Create(const std::string &uristr);

    /**
     * @brief URI 构造函数
     *
     */
    Uri();

public:  /// GET 方法
    const std::string &getScheme() const { return m_scheme; }
    const std::string &getUserinfo() const { return m_userinfo; }
    const std::string &getHost() const { return m_host; }
    const std::string &getPath() const;
    const std::string &getQuery() const { return m_query; }
    const std::string &getFragment() const { return m_fragment; }
    int32_t getPort() const;

public:  /// SET 方法
    void setScheme(const std::string &v) { m_scheme = v; }
    void setUserinfo(const std::string &v) { m_userinfo = v; }
    void setHost(const std::string &v) { m_host = v; }
    void setPath(const std::string &v) { m_path = v; }
    void setQuery(const std::string &v) { m_query = v; }
    void setFragment(const std::string &v) { m_fragment = v; }
    void setPort(const int32_t &v) { m_port = v; }

public:  /// 辅助方法
    std::ostream &dump(std::ostream &os) const;
    std::string toString() const;

    Address::ptr createAddress() const;

private:
    bool isDefaultPort() const;

private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    int32_t m_port;
};

}  // namespace ljrserver

#endif  //__LJRSERVER_URI_H__