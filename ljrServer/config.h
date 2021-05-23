
#ifndef __LJRSERVER_CONFIG_H__
#define __LJRSERVER_CONFIG_H__

// 智能指针
#include <memory>
// 字符串
#include <string>
#include <sstream>
// 类型转换
#include <boost/lexical_cast.hpp>
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

namespace ljrserver
{

    // 配置项基类
    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;

        ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name), m_description(description)
        {
            // 转小写
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase() {}

        const std::string &getName() const { return m_name; }
        const std::string &getDescription() const { return m_description; }

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string &str) = 0;

        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;
        std::string m_description;
    };

    // 基础类型转换
    // F from_type, T to_type
    template <class F, class T>
    class LexicalCast
    {
    public:
        T operator()(const F &v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    // vector
    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // list
    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // set
    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // unordered_set
    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // map
    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
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
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
                // node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // FromStr T operator()(const std::string &)
    // ToStr   std::string operator()(const T &)
    template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        typedef RWMutex RWMutexType;
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

        ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
            : ConfigVarBase(name, description), m_val(default_value) {}

        std::string toString() override
        {
            try
            {
                RWMutexType::ReadLock lock(m_mutex);

                // return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val);
            }
            catch (const std::exception &e)
            {
                LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                                                          << " convert: " << typeid(m_val).name() << " to string";
            }
            return "";
        }

        bool fromString(const std::string &str) override
        {
            try
            {
                // m_val = boost::lexical_cast<T>(str);
                setValue(FromStr()(str));
                return true;
            }
            catch (const std::exception &e)
            {
                LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                                                          << " convert: string to " << typeid(m_val).name()
                                                          << " - " << str;
            }
            return false;
        }

        // const T getValue() const
        const T getValue()
        {
            // 加了const函数 不能枷锁 会修改成员变量
            RWMutexType::ReadLock lock(m_mutex);

            return m_val;
        }

        void setValue(const T &v)
        {
            {
                RWMutexType::ReadLock lock(m_mutex);

                if (v == m_val)
                {
                    return;
                }
                for (auto &i : m_cbs)
                {
                    // 回调函数，提醒配置变化 m_val -> v
                    i.second(m_val, v);
                }
            }
            RWMutexType::WriteLock lock(m_mutex);

            m_val = v;
        }

        // 返回模版类 T 的类型名称
        std::string getTypeName() const override { return typeid(T).name(); }

        // void addListener(uint64_t key, on_change_cb cb)
        // {
        //     m_cbs[key] = cb;
        // }

        // 添加监听函数，返回函数id
        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0;
            RWMutexType::WriteLock lock(m_mutex);

            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        // 删除监听函数，使用函数id作为key值删除
        void delListener(uint64_t key)
        {
            RWMutexType::WriteLock lock(m_mutex);

            m_cbs.erase(key);
        }

        // 删除所有监听函数
        void clearListener()
        {
            RWMutexType::WriteLock lock(m_mutex);

            m_cbs.clear();
        }

        // 通过函数唯一id作为key值，从map中返回函数对象
        on_change_cb getListener(uint64_t key)
        {
            RWMutexType::ReadLock lock(m_mutex);

            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

    private:
        // 配置项
        T m_val;
        // 变更回调函数组 uint64_t key要求唯一，一般可用hash
        std::map<uint64_t, on_change_cb> m_cbs;
        // 读写锁
        RWMutexType m_mutex;
    };

    class Config
    {
    public:
        typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        typedef RWMutex RWMutexType;

        // 获取/创建配置项
        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_value, const std::string &description = "")
        {
            RWMutexType::WriteLock lock(GetMutex());

            auto it = GetDatas().find(name);
            if (it != GetDatas().end())
            {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp)
                {
                    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "Lookup name = " << name << " exists.";
                    return tmp;
                }
                else
                {
                    LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT()) << "Lookup name: " << name
                                                              << " exists but type not "
                                                              << typeid(T).name()
                                                              << " real_type = "
                                                              << it->second->getTypeName()
                                                              << " " << it->second->toString();
                    return nullptr;
                }
            }

            // auto tmp = Lookup<T>(name);
            // if (tmp)
            // {
            //     LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "Lookup name = " << name << " exists.";
            //     return tmp;
            // }

            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos)
            {
                LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT()) << "Lookup name invalid: " << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        // 查找配置项，只传入配置参数名
        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            RWMutexType::ReadLock lock(GetMutex());

            auto it = GetDatas().find(name);
            if (it == GetDatas().end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        // 使用YAML::Node初始化配置模块
        static void LoadFromYaml(const YAML::Node &root);

        // 查找配置参数，返回配置参数的基类
        static ConfigVarBase::ptr LookupBase(const std::string &name);

        // 遍历配置模块里面所有配置项
        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    private:
        static ConfigVarMap &GetDatas()
        {
            // 防止静态变量没被初始化，确保先被初始化
            static ConfigVarMap s_datas;
            return s_datas;
        }
        // static ConfigVarMap s_datas;

        static RWMutexType &GetMutex()
        {
            // 全局配置 Config
            // 全局静态变量，初始化没有严格的顺序
            // 要确保使用锁前，静态锁变量已经被初始化
            static RWMutexType s_mutex;
            return s_mutex;
        }
    };

}

#endif