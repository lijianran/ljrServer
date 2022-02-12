
#include "../ljrServer/uri.h"
#include <iostream>

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    // 创建 URI
    ljrserver::Uri::ptr uri = ljrserver::Uri::Create(
        "http://www.sylar.top/test/uri?id=100&name=lijianran#fra");

    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
