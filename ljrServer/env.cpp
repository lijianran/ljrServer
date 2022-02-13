/**
 * @file env.cpp
 * @author lijianran (lijianran@outlook.com)
 * @brief 环境参数
 * @version 0.1
 * @date 2022-02-12
 */

#include "env.h"

// 日志
#include "log.h"
// 字符串
#include <string.h>
// IO 输出
#include <iostream>
// 输出格式
#include <iomanip>
// readlink
#include <unistd.h>
// setenv getenv
#include <stdlib.h>

namespace ljrserver {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/**
 * @brief 初始化解析参数
 *
 * @param argc
 * @param argv
 * @return true
 * @return false
 */
bool Env::init(int argc, char** argv) {
    char link[1024] = {0};
    char path[1024] = {0};
    // exe 软连接 ln
    sprintf(link, "/proc/%d/exe", getpid());
    // 绝对路径
    readlink(link, path, sizeof(path));
    m_exe = path;

    // 找到最后一个 /
    auto pos = m_exe.find_last_of("/");
    m_cwd = m_exe.substr(0, pos) + "/";

    // -config /path/to/config -file xxx -d
    // 程序名称
    m_program = argv[0];

    const char* now_key = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            // key
            if (strlen(argv[0]) > 1) {
                if (now_key) {
                    add(now_key, "");
                }
                now_key = argv[i] + 1;

            } else {
                LJRSERVER_LOG_ERROR(g_logger)
                    << "invalid arg index=" << i << " val=" << argv[i];
                return false;
            }
        } else {
            if (now_key) {
                add(now_key, argv[i]);
                now_key = nullptr;

            } else {
                LJRSERVER_LOG_ERROR(g_logger)
                    << "invalid arg index=" << i << " val=" << argv[i];
                return false;
            }
        }
    }

    // 最后一个参数 -p 没有 val
    if (now_key) {
        add(now_key, "");
    }

    return true;
}

/**
 * @brief 增加
 *
 * @param key
 * @param val
 */
void Env::add(const std::string& key, const std::string& val) {
    // 写锁
    RWMutexType::WriteLock lock(m_mutex);
    m_args[key] = val;
}

/**
 * @brief 是否有
 *
 * @param key
 * @return true
 * @return false
 */
bool Env::has(const std::string& key) {
    // 读锁
    RWMutexType::ReadLock loc(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end();
}

/**
 * @brief 删除
 *
 * @param key
 */
void Env::del(const std::string& key) {
    // 写锁
    RWMutexType::WriteLock lock(m_mutex);
    m_args.erase(key);
}

/**
 * @brief 获取
 *
 * @param key
 * @param val
 * @return std::string
 */
std::string Env::get(const std::string& key, const std::string& default_val) {
    // 读锁
    RWMutexType::ReadLock loc(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end() ? it->second : default_val;
}

/**
 * @brief 增加说明
 *
 * @param key
 * @param desc
 */
void Env::addHelp(const std::string& key, const std::string& desc) {
    // 先删除
    delHelp(key);
    // 写锁
    RWMutexType::WriteLock lock(m_mutex);
    m_helps.push_back(std::make_pair(key, desc));
}

/**
 * @brief 删除说明
 *
 * @param key
 */
void Env::delHelp(const std::string& key) {
    // 写锁
    RWMutexType::WriteLock lock(m_mutex);
    for (auto it = m_helps.begin(); it != m_helps.end();) {
        if (it->first == key) {
            it = m_helps.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * @brief 打印说明
 *
 */
void Env::printHelp() {
    // 读锁
    RWMutexType::ReadLock lock(m_mutex);
    // 程序使用说明
    std::cout << "Usage: " << m_program << " [options]" << std::endl;

    for (auto& i : m_helps) {
        std::cout << std::setw(5) << "-" << i.first << " : " << i.second
                  << std::endl;
    }
}

/**
 * @brief 设置环境变量
 *
 * @param key
 * @param val
 * @return true
 * @return false
 */
bool Env::setEnv(const std::string& key, const std::string& val) {
    return !setenv(key.c_str(), val.c_str(), 1);
}

/**
 * @brief 获取环境变量
 *
 * @param key
 * @param default_val
 * @return std::string
 */
std::string Env::getEnv(const std::string& key,
                        const std::string& default_val) {
    const char* v = getenv(key.c_str());
    if (v == nullptr) {
        return default_val;
    }
    return v;
}

/**
 * @brief 获取相对 cwd 的绝对路径
 *
 * @param path
 * @return std::string
 */
std::string Env::getAbsolutePath(const std::string& path) const {
    if (path.empty()) {
        return "/";
    }
    if (path[0] == '/') {
        // 已经是绝对路径
        return path;
    }
    return m_cwd + path;
}

/**
 * @brief 获取配置文件夹路径
 *
 * @return std::string
 */
std::string Env::getConfigPath() {
    // 获取用户 c 输入值 没有则使用默认路径 conf
    return getAbsolutePath(get("c", "conf"));
}

};  // namespace ljrserver