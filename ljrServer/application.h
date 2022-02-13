/**
 * @file application.h
 * @author lijianran (lijianran@outlook.com)
 * @brief 应用
 * @version 0.1
 * @date 2022-02-13
 */
#pragma once

#include "http/http_server.h"

#include <vector>

namespace ljrserver {

class Application {
public:
    Application();

    static Application* GetInstance() { return s_instance; }

    bool init(int argc, char** argv);

    bool run();

private:
    int main(int argc, char** argv);

    int run_fiber();

private:
    int m_argc = 0;

    char** m_argv = nullptr;

    static Application* s_instance;

    std::vector<ljrserver::http::HttpServer::ptr> m_httpservers;
};

};  // namespace ljrserver