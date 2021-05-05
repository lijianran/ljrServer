
#include "../ljrServer/uri.h"
#include <iostream>

int main(int argc, char const *argv[])
{
    ljrserver::Uri::ptr uri = ljrserver::Uri::Create("http://www.sylar.top/test/uri?id=100&name=lijianran#fra");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
