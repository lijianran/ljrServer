
#include "../ljrServer/ljrserver.h"

ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

void run_in_fiber()
{
    LJRSERVER_LOG_INFO(g_logger) << "run_in_fiber begin";

    ljrserver::Fiber::YieldToHold();

    LJRSERVER_LOG_INFO(g_logger) << "run_in_fiber end";

    ljrserver::Fiber::YieldToHold();
}

void test_fiber()
{
    LJRSERVER_LOG_INFO(g_logger) << "main begin - 1";

    {
        ljrserver::Fiber::GetThis();
        LJRSERVER_LOG_INFO(g_logger) << "main begin";
        ljrserver::Fiber::ptr fiber(new ljrserver::Fiber(run_in_fiber));
        fiber->swapIn();
        LJRSERVER_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        LJRSERVER_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }

    LJRSERVER_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char const *argv[])
{
    ljrserver::Thread::SetName("main");

    // 测试多线程多协程
    std::vector<ljrserver::Thread::ptr> thrs;
    for (int i = 0; i < 3; i++)
    {
        thrs.push_back(ljrserver::Thread::ptr(
            new ljrserver::Thread(test_fiber, "name_"+std::to_string(i))));
    }

    for (auto i : thrs)
    {
        i->join();
    }
    
    return 0;
}
