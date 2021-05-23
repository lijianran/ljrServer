
// #include "../ljrServer/ljrserver.h"
#include "../ljrServer/util.h"
#include "../ljrServer/log.h"

ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

void test_assert()
{
    // 测试assert
    // assert(0);

    // 测试函数调用栈
    LJRSERVER_LOG_INFO(g_logger) << ljrserver::BacktraceToString(10);
    
    // 测试macro 断言宏定义
    // LJRSERVER_ASSERT(false);
    // LJRSERVER_ASSERT2(0 == 1, "abcdefghijk lmn");


}

int main(int argc, char const *argv[])
{
    test_assert();
    
    return 0;
}
