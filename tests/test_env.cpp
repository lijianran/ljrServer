/**
 * @file test_env.cpp
 * @author lijianran (lijianran@outlook.com)
 * @brief 测试环境参数
 * @version 0.1
 * @date 2022-02-12
 */

#include "ljrServer/env.h"

#include <iostream>
#include <fstream>
#include <unistd.h>

struct Init {
    Init() {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline",
                          std::ios::binary);

        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        // std::cout << content << std::endl;
        for (size_t i = 0; i < content.size(); ++i) {
            std::cout << i << " - " << content[i] << " - " << (int)content[i]
                      << std::endl;
        }
    }
};

// 在 main 函数之前获取输入参数
Init init;

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char** argv) {
    std::cout << "argc=" << argc << std::endl;

    // 增加 help 参数说明
    ljrserver::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    ljrserver::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    ljrserver::EnvMgr::GetInstance()->addHelp("p", "print help");

    // 解析参数
    if (!ljrserver::EnvMgr::GetInstance()->init(argc, argv)) {
        // 解析失败
        ljrserver::EnvMgr::GetInstance()->printHelp();
    }

    std::cout << "exe=" << ljrserver::EnvMgr::GetInstance()->getExe()
              << std::endl;
    std::cout << "exe=" << ljrserver::EnvMgr::GetInstance()->getCwd()
              << std::endl;

    // 是否有某个参数
    if (ljrserver::EnvMgr::GetInstance()->has("p")) {
        ljrserver::EnvMgr::GetInstance()->printHelp();
    }

    /**
     * @brief 测试环境变量
     *
     */
    // echo $PATH
    std::cout << "path="
              << ljrserver::EnvMgr::GetInstance()->getEnv("PATH", "xxx")
              << std::endl;

    std::cout << "test="
              << ljrserver::EnvMgr::GetInstance()->getEnv("TEST", "xx")
              << std::endl;

    std::cout << "set env "
              << ljrserver::EnvMgr::GetInstance()->setEnv("TEST", "yy")
              << std::endl;

    std::cout << "test="
              << ljrserver::EnvMgr::GetInstance()->getEnv("TEST", "xx")
              << std::endl;

    return 0;
}