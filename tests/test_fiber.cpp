
// #include "../ljrServer/ljrserver.h"
#include "../ljrServer/log.h"
#include "../ljrServer/fiber.h"

ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

void run_in_fiber()
{
    LJRSERVER_LOG_INFO(g_logger) << "run_in_fiber begin";

    // ljrserver::Fiber::YieldToHold();
    ljrserver::Fiber::GetThis()->back();

    LJRSERVER_LOG_INFO(g_logger) << "run_in_fiber end";

    // ljrserver::Fiber::YieldToHold();
    ljrserver::Fiber::GetThis()->back();

}

void test_fiber()
{
    LJRSERVER_LOG_INFO(g_logger) << "main begin - 1";

    {
        ljrserver::Fiber::GetThis();
        LJRSERVER_LOG_INFO(g_logger) << "main begin";
        ljrserver::Fiber::ptr fiber(new ljrserver::Fiber(&run_in_fiber, 0, true));
        fiber->call();
        LJRSERVER_LOG_INFO(g_logger) << "main after swapIn";
        fiber->call();
        LJRSERVER_LOG_INFO(g_logger) << "main after end";
        fiber->call();
    }

    LJRSERVER_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char const *argv[])
{
    ljrserver::Thread::SetName("main");

    // 测试多线程多协程
    std::vector<ljrserver::Thread::ptr> thrs;
    for (int i = 0; i < 1; i++)
    {
        thrs.push_back(ljrserver::Thread::ptr(new ljrserver::Thread(&test_fiber, "name_" + std::to_string(i))));
    }

    for (auto i : thrs)
    {
        i->join();
    }

    return 0;
}
