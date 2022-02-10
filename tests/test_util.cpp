
// #include "../ljrServer/ljrserver.h"
#include "../ljrServer/util.h"
#include "../ljrServer/log.h"
#include "../ljrServer/macro.h"

// 日志
ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 测试断言 函数调用栈
 *
 */
void test_assert() {
    // 测试 assert
    // assert(0);

    // 测试函数调用栈
    // LJRSERVER_LOG_INFO(g_logger) << ljrserver::BacktraceToString(10);

    // 测试 macro 断言宏定义
    // LJRSERVER_ASSERT(false);
    LJRSERVER_ASSERT2(0 == 1, "这里写要输出的内容: abcdefghijk lmn");
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    // 测试断言 函数调用栈
    test_assert();

    return 0;
}
