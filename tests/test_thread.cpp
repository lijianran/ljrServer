
#include "../ljrServer/ljrserver.h"
#include "../ljrServer/thread.h"

ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

int count = 0;
// ljrserver::RWMutex s_mutex;
ljrserver::Mutex s_mutex;

void fun1()
{
    LJRSERVER_LOG_INFO(g_logger) << "name: " << ljrserver::Thread::GetName()
                                 << " this.name: " << ljrserver::Thread::GetThis()->getName()
                                 << " id: " << ljrserver::GetThreadId()
                                 << " this.id: " << ljrserver::Thread::GetThis()->getId();

    for (int i = 0; i < 100000; i++)
    {
        // ljrserver::RWMutex::WriteLock lock(s_mutex);
        ljrserver::Mutex::Lock lock(s_mutex);

        ++count;
    }
}

void fun2()
{
    while (true)
    {
        LJRSERVER_LOG_INFO(g_logger) << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    }
}

void fun3()
{
    while (true)
    {
        LJRSERVER_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char const *argv[])
{
    LJRSERVER_LOG_INFO(g_logger) << "开始测试线程";
    std::vector<ljrserver::Thread::ptr> thrs;
    // 测试线程
    // for (int i = 0; i < 5; ++i)
    // {
    //     ljrserver::Thread::ptr thr(new ljrserver::Thread(&fun1, "name" + std::to_string(i)));
    //     thrs.push_back(thr);
    // }

    // 测试锁
    YAML::Node root = YAML::LoadFile("/ljrServer/bin/conf/log2.yml");
    ljrserver::Config::LoadFromYaml(root);

    for (int i = 0; i < 2; i++)
    {
        ljrserver::Thread::ptr thr(new ljrserver::Thread(&fun2, "name_" + std::to_string(i * 2)));
        ljrserver::Thread::ptr thr2(new ljrserver::Thread(&fun3, "name_" + std::to_string(i * 2)));

        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for (size_t i = 0; i < thrs.size(); ++i)
    {
        thrs[i]->join();
    }

    LJRSERVER_LOG_INFO(g_logger) << "结束测试线程";

    LJRSERVER_LOG_INFO(g_logger) << "count = " << count;


    return 0;
}
