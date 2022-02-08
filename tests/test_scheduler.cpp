
// #include "../ljrServer/ljrserver.h"
#include "../ljrServer/log.h"
#include "../ljrServer/scheduler.h"

ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

/**
 * @brief 协程执行函数
 *
 */
void test_fiber() {
    static int s_count = 5;

    LJRSERVER_LOG_INFO(g_logger) << "test in fiber s_count = " << s_count;

    sleep(1);
    if (--s_count >= 0) {
        ljrserver::Scheduler::GetThis()->schedule(&test_fiber);

        // 指定在当前线程执行
        // ljrserver::Scheduler::GetThis()->schedule(&test_fiber,
        //                                           ljrserver::GetThreadId());
    }
}

/**
 * @brief 测试协程调度 Scheduler
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    ljrserver::Thread::SetName("caller");

    ljrserver::Scheduler sc(1, false, "sc");

    // sc.schedule(&test_fiber);

    sc.start();
    sleep(2);

    sc.schedule(&test_fiber);

    sc.stop();

    LJRSERVER_LOG_INFO(g_logger) << "over";

    return 0;
}
