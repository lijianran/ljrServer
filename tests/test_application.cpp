/**
 * @file test_application.cpp
 * @author lijianran (lijianran@outlook.com)
 * @brief 测试应用
 * @version 0.1
 * @date 2022-02-13
 */

#include "ljrServer/application.h"

/**
 * @brief 测试应用程序
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char** argv) {
    // 应用实例
    ljrserver::Application app;
    // 初始化解析参数
    if (app.init(argc, argv)) {
        // 运行应用
        return app.run();
    }
    // 参数错误
    return 0;
}
