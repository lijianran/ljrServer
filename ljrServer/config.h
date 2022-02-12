
#ifndef __LJRSERVER_CONFIG_H__
#define __LJRSERVER_CONFIG_H__

// 智能指针
#include <memory>
// 字符串
#include <string>
// 字符流
#include <sstream>
// 类型转换
#include <boost/lexical_cast.hpp>
// yaml
#include <yaml-cpp/yaml.h>
// 容器偏特化
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
// 函数
#include <functional>
// 日志
#include "log.h"
// 线程
#include "thread.h"

namespace ljrserver {

/**
 * @brief Class 配置项基类
 *
 * 抽象类 不能实例化对象
 */
class ConfigVarBase {
public:
    // 智能指针
    typedef std::shared_ptr<ConfigVarBase> ptr;

    /**
     * @brief Construct a new Config Var Base object
     *
     * @param name 配置名自动转小写
     * @param description 配置描述
     */
    ConfigVarBase(const std::string &name, const std::string &description = "")
        : m_name(name), m_description(description) {
        // 转小写
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    /**
     * @brief Destroy the Config Var Base object
     *
     * 基类析构函数需要设置为虚函数
     */
    virtual ~ConfigVarBase() {}

    /**
     * @brief Get the Name object 获取配置项名称
     *
     * @return const std::string&
     */
    const std::string &getName() const { return m_name; }

    /**
     * @brief Get the Description object 获取配置项的描述
     *
     * @return const std::string&
     */
    const std::string &getDescription() const { return m_description; }

    /**
     * @brief 纯虚函数 config -> 字符串
     *
     * @return std::string
     */
    virtual std::string toString() = 0;

    /**
     * @brief 纯虚函数 字符串 -> config
     *
     * @param str
     * @return true
     * @return false
     */
    virtual bool fromString(const std::string &str) = 0;

    /**
     * @brief 纯虚函数 获取类型名称
     *
     * @return std::string
     */
    virtual std::string getTypeName() const = 0;

protected:
    // 配置项名称
    std::string m_name;

    // 描述
    std::string m_description;
};

/**
 * @brief 基础类型转换
 *
 * boost::lexical_cast<T>(F)
 *
 * @tparam F from_type
 * @tparam T to_type
 */
template <class F, class T>
class LexicalCast {
public:
    T operator()(const F &v) { return boost::lexical_cast<T>(v); }
};

/**************
 * 容器偏特化
 **************/

// vector
template <class T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); i++) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// list
template <class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); i++) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// set
template <class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); i++) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// unordered_set
template <class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); i++) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// map
template <class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                      LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            // node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// unordered_map
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                      LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            // node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief Class 配置项
 *
 * 模版类
 *
 * @tparam T
 * @tparam FromStr T operator()(const std::string &)
 * @tparam ToStr   std::string operator()(const T &)
 */
template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    // 智能指针
    typedef std::shared_ptr<ConfigVar> ptr;

    // 读写锁
    typedef RWMutex RWMutexType;

    // 监听回调函数
    typedef std::function<void(const T &old_value, const T &new_value)>
        on_change_cb;

    /**
     * @brief Construct a new Config Var object
     *
     * @param name 配置项名称
     * @param default_value 默认值
     * @param description 描述
     */
    ConfigVar(const std::string &name, const T &default_value,
              const std::string &description = "")
        : ConfigVarBase(name, description), m_val(default_value) {}

    /**
     * @brief 纯虚函数的实现 config -> 字符串
     *
     * @return std::string 配置字符串
     */
    std::string toString() override {
        try {
            RWMutexType::ReadLock lock(m_mutex);

            // return boost::lexical_cast<std::string>(m_val);
            // 仿函数 ToStr()(m_val);
            return ToStr()(m_val);
        } catch (const std::exception &e) {
            LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT())
                << "ConfigVar::toString exception" << e.what()
                << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

    /**
     * @brief 纯虚函数的实现 字符串 -> config
     *
     * @param str 配置字符串
     * @return true
     * @return false
     */
    bool fromString(const std::string &str) override {
        try {
            // m_val = boost::lexical_cast<T>(str);
            // 仿函数 FromStr()(str);
            setValue(FromStr()(str));
            return true;
        } catch (const std::exception &e) {
            LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT())
                << "ConfigVar::toString exception" << e.what()
                << " convert: string to " << typeid(m_val).name() << " - "
                << str;
        }
        return false;
    }

    /**
     * @brief 纯虚函数的实现 返回模版类 T 的类型名称
     *
     * @return std::string
     */
    std::string getTypeName() const override { return typeid(T).name(); }

    /**
     * @brief 获取配置项的值
     *
     * @return const T
     */
    const T getValue() {
        // const T getValue() const { // const 无法上锁 }
        // 加了 const 限制函数 不能枷锁 会修改成员变量
        RWMutexType::ReadLock lock(m_mutex);

        return m_val;
    }

    /**
     * @brief 设置配置项的值
     *
     * 变更监听
     *
     * @param v
     */
    void setValue(const T &v) {
        {
            // 局部上读锁
            RWMutexType::ReadLock lock(m_mutex);

            if (v == m_val) {
                return;
            }
            for (auto &i : m_cbs) {
                // 回调函数，提醒配置变化 m_val -> v
                i.second(m_val, v);
            }
        }
        // 上写锁
        RWMutexType::WriteLock lock(m_mutex);

        m_val = v;
    }

    // void addListener(uint64_t key, on_change_cb cb)
    // {
    //     m_cbs[key] = cb;
    // }

    /**
     * @brief 添加监听函数，返回函数 id
     *
     * @param cb 回调函数
     * @return uint64_t 监听 id
     */
    uint64_t addListener(on_change_cb cb) {
        // 局部静态变量
        static uint64_t s_fun_id = 0;

        // 上写锁
        RWMutexType::WriteLock lock(m_mutex);

        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    /**
     * @brief 删除监听函数，使用函数 id 作为 key 值删除
     *
     * @param key 监听 id
     */
    void delListener(uint64_t key) {
        // 上写锁
        RWMutexType::WriteLock lock(m_mutex);

        m_cbs.erase(key);
    }

    /**
     * @brief 删除所有监听函数
     *
     */
    void clearListener() {
        // 上写锁
        RWMutexType::WriteLock lock(m_mutex);

        m_cbs.clear();
    }

    /**
     * @brief 通过函数唯一 id 作为 key 值，从 map 中返回函数对象
     *
     * @param key 监听 id
     * @return on_change_cb 回调函数
     */
    on_change_cb getListener(const uint64_t key) const {
        // 上读锁
        RWMutexType::ReadLock lock(m_mutex);

        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

private:
    // 配置项的值
    T m_val;

    // 变更回调函数组 uint64_t key 要求唯一 一般可用 hash
    std::map<uint64_t, on_change_cb> m_cbs;

    // 读写锁
    RWMutexType m_mutex;
};

/**
 * @brief Class 配置
 */
class Config {
public:
    // 配置项 map
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    // 读写锁
    typedef RWMutex RWMutexType;

    /**
     * @brief 获取配置项 没有则创建
     *
     * 模版函数 定义与实现不要分离 避免链接出错
     *
     * @tparam T
     * @param name 配置项名称
     * @param default_value 默认值
     * @param description 描述
     * @return ConfigVar<T>::ptr 配置项指针
     */
    template <class T>
    static typename ConfigVar<T>::ptr Lookup(
        const std::string &name, const T &default_value,
        const std::string &description = "") {
        // 上写锁
        RWMutexType::WriteLock lock(GetMutex());

        // find key
        auto it = GetDatas().find(name);
        if (it != GetDatas().end()) {
            // 动态转换
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if (tmp) {
                // 找到了配置项
                LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
                    << "Lookup name = " << name << " exists.";
                return tmp;
            } else {
                // 找到配置项但类型不对 转换失败
                LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT())
                    << "Lookup name: " << name << " exists but type not "
                    << typeid(T).name()
                    << " real_type = " << it->second->getTypeName() << " "
                    << it->second->toString();
                return nullptr;
            }
        }

        // auto tmp = Lookup<T>(name);
        // if (tmp)
        // {
        //     LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "Lookup name = " <<
        //     name << " exists.";
        //     return tmp;
        // }

        if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") !=
            std::string::npos) {
            // 配置项名称错误
            LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT())
                << "Lookup name invalid: " << name;
            throw std::invalid_argument(name);
        }

        // 创建配置项
        typename ConfigVar<T>::ptr v(
            new ConfigVar<T>(name, default_value, description));

        // 存入配置 map
        GetDatas()[name] = v;
        return v;
    }

    /**
     * @brief 获取配置项
     *
     * 参数列表不同  重载
     * 模版函数 定义与实现不要分离 避免链接出错
     *
     * @tparam T
     * @param name 配置项名称
     * @return ConfigVar<T>::ptr 配置项指针
     */
    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
        // 上读锁
        RWMutexType::ReadLock lock(GetMutex());

        auto it = GetDatas().find(name);
        if (it == GetDatas().end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    /**
     * @brief 使用 YAML::Node 初始化配置模块
     *
     *  YAML::Node root = YAML::LoadFile("/ljrServer/bin/conf/test.yml");
        ljrserver::Config::LoadFromYaml(root);

     * @param root
     */
    static void LoadFromYaml(const YAML::Node &root);

    /**
     * @brief 查找配置参数，返回配置参数的基类
     *
     * @param name 配置项名称
     * @return ConfigVarBase::ptr
     */
    static ConfigVarBase::ptr LookupBase(const std::string &name);

    /**
     * @brief 遍历配置模块里所有的配置项
     *
     * @param cb 访问函数
     */
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
    // 不能确保使用前已经初始化
    // static ConfigVarMap s_datas;

    /**
     * @brief 获取配置项 map
     *
     * @return ConfigVarMap&
     */
    static ConfigVarMap &GetDatas() {
        // 防止静态变量没被初始化，确保先被初始化
        static ConfigVarMap s_datas;
        return s_datas;
    }

    /**
     * @brief 获取锁
     *
     * @return RWMutexType&
     */
    static RWMutexType &GetMutex() {
        // 全局配置 Config
        // 全局静态变量，初始化没有严格的顺序
        // 要确保使用锁前，静态锁变量已经被初始化
        static RWMutexType s_mutex;
        return s_mutex;
    }
};

}  // namespace ljrserver

#endif  // __LJRSERVER_CONFIG_H__