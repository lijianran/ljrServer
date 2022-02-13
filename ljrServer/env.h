/**
 * @file env.h
 * @author lijianran (lijianran@outlook.com)
 * @brief 环境参数
 * @version 0.1
 * @date 2022-02-12
 */

#pragma once

#include "singleton.h"
#include "thread.h"

#include <map>
#include <string>
#include <vector>

namespace ljrserver {

/**
 * @brief Class 环境变量
 *
 */
class Env {
public:
    // 读写锁
    typedef RWMutex RWMutexType;

    bool init(int argc, char** argv);

    void add(const std::string& key, const std::string& val);

    bool has(const std::string& key);

    void del(const std::string& key);

    std::string get(const std::string& key,
                    const std::string& default_val = "");

    void addHelp(const std::string& key, const std::string& desc);

    void delHelp(const std::string& key);

    void printHelp();

public:
    const std::string& getExe() const { return m_exe; }

    const std::string& getCwd() const { return m_cwd; }

    bool setEnv(const std::string& key, const std::string& val);

    std::string getEnv(const std::string& key,
                       const std::string& default_val = "");

public:
    std::string getAbsolutePath(const std::string& path) const;

    std::string getConfigPath();

private:
    // 读写锁
    RWMutexType m_mutex;

    // 参数字典
    std::map<std::string, std::string> m_args;

    // help 参数说明
    std::vector<std::pair<std::string, std::string>> m_helps;

    // 程序名称
    std::string m_program;

    // 程序二进制文件路径 绝对路径
    std::string m_exe;

    // 工作路径
    std::string m_cwd;
};

// 单例模式
typedef ljrserver::Singleton<Env> EnvMgr;

};  // namespace ljrserver