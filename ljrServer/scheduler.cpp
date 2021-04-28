
#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace ljrserver
{

    static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

    // 线程局部变量
    static thread_local Scheduler *t_scheduler = nullptr;
    static thread_local Fiber *t_fiber = nullptr;

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) : m_name(name)
    {
        LJRSERVER_ASSERT(threads > 0);

        if (use_caller)
        {
            ljrserver::Fiber::GetThis();
            --threads;

            // 防止已经有调度器
            LJRSERVER_ASSERT(GetThis() == nullptr);
            t_scheduler = this;

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            ljrserver::Thread::SetName(m_name);

            t_fiber = m_rootFiber.get();

            m_rootThread = ljrserver::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        }
        else
        {
            m_rootThread = -1;
        }

        m_threadCount = threads;
    }

    Scheduler::~Scheduler()
    {
        LJRSERVER_ASSERT(m_stopping);

        if (GetThis() == this)
        {
            t_scheduler = nullptr;
        }
    }

    // 获取当前调度器
    Scheduler *Scheduler::GetThis()
    {
        return t_scheduler;
    }

    // 获取主协程
    Fiber *Scheduler::GetMainFiber()
    {
        return t_fiber;
    }

    // 启动线程池
    void Scheduler::start()
    {
        MutexType::Lock lock(m_mutex);
        if (!m_stopping)
        {
            return;
        }
        m_stopping = false;

        LJRSERVER_ASSERT(m_threads.empty());

        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                          m_name + "_" + std::to_string(i)));

            // Thread 构造函数中加了信号量，确保线程id会初始化，这里能获取正确的线程id
            m_threadIds.push_back(m_threads[i]->getId());
        }
        lock.unlock();

        // if (m_rootFiber)
        // {
        //     m_rootFiber->call();
        // }
    }

    void Scheduler::stop()
    {
        m_autoStop = true;
        if (m_rootFiber && m_threadCount == 0 &&
            (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
        {
            LJRSERVER_LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;

            if (stopping())
            {
                return;
            }
        }

        // bool exit_on_this_fiber = false;
        if (m_rootThread != -1)
        {
            LJRSERVER_ASSERT(GetThis() == this);
        }
        else
        {
            LJRSERVER_ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            // while (!stopping())
            // {
            //     if (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::EXCEPT)
            //     {
            //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            //         LJRSERVER_LOG_INFO(g_logger) << " root fiber is term, reset";
            //         t_fiber = m_rootFiber.get();
            //     }
            //     m_rootFiber->call();
            // }
            if (!stopping())
            {
                m_rootFiber->call();
            }
        }

        std::vector<Thread::ptr> thrs;

        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for (auto &i : thrs)
        {
            i->join();
        }

        // if (stopping())
        // {
        //     return;
        // }

        // if (exit_on_this_fiber)
        // {
        // }
    }

    void Scheduler::setThis()
    {
        t_scheduler = this;
    }

    void Scheduler::run()
    {
        LJRSERVER_LOG_DEBUG(g_logger) << "run";

        // 设置当前线程的协程调度器t_scheduler
        setThis();

        // 设置当前线程执行的协程t_fiber
        if (ljrserver::GetThreadId() != m_rootThread)
        {
            t_fiber = Fiber::GetThis().get();
        }

        // 闲置协程
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        // 回调函数协程
        Fiber::ptr cb_fiber;
        // 协程调度，协程/回调函数
        FiberAndThread ft;

        // 协程调度循环
        while (true)
        {
            ft.reset();

            bool tickle_me = false;
            bool is_active = false;

            {
                // 在协程队列中找需要执行的协程任务，需要枷锁
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    // 有指定线程id，但不是当前线程，continue，同时提醒其他线程处理
                    if (it->thread != -1 && it->thread != ljrserver::GetThreadId())
                    {
                        ++it;
                        tickle_me = true;
                        continue;
                    }

                    LJRSERVER_ASSERT(it->fiber || it->cb);
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC)
                    {
                        // 正在执行的协程，continue
                        ++it;
                        continue;
                    }

                    // 取出协程
                    ft = *it;
                    m_fibers.erase(it);
                    ++m_activeThreadCount;
                    is_active = true;
                    break;
                }
            }

            // 提醒其他线程
            if (tickle_me)
            {
                tickle();
            }

            if (ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT))
            {
                // 状态不是终止和意外，执行协程
                ft.fiber->swapIn();
                --m_activeThreadCount;

                // 协程出来后，若状态位ready，则继续进协程队列等待处理
                if (ft.fiber->getState() == Fiber::READY)
                {
                    schedule(ft.fiber);
                }
                else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
                {
                    // 如果协程不是终止或意外，则设为hold挂起状态
                    ft.fiber->m_state = Fiber::HOLD;
                }
                ft.reset();
            }
            else if (ft.cb)
            {
                // 如果是回调函数形式
                // 判断回调函数协程是否已经有内存
                if (cb_fiber)
                {
                    // 重置
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    // 新建
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();

                // 执行协程
                cb_fiber->swapIn();
                --m_activeThreadCount;

                // 协程出来后，状态是ready，则进入协程队列
                if (cb_fiber->getState() == Fiber::READY)
                {
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }
                else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM)
                {
                    // 终止和意外的情况，重置为空
                    cb_fiber->reset(nullptr);
                }
                else // if (cb_fiber->getState() != Fiber::TERM)
                {
                    // 其他状态，设为hold挂起
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            }
            else
            {
                if (is_active)
                {
                    --m_activeThreadCount;
                    continue;
                }

                // 协程队列中没有待处理的协程，则使用idle协程占用cpu
                if (idle_fiber->getState() == Fiber::TERM)
                {
                    LJRSERVER_LOG_INFO(g_logger) << "idle fiber terminate";
                    break;
                }

                ++m_idleThreadCount;
                idle_fiber->swapIn();
                --m_idleThreadCount;
                if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT)
                {
                    idle_fiber->m_state = Fiber::HOLD;
                }
            }
        }
    }

    void Scheduler::tickle()
    {
        LJRSERVER_LOG_INFO(g_logger) << "tickle";
    }

    bool Scheduler::stopping()
    {
        MutexType::Lock lock(m_mutex);
        return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::idle()
    {
        LJRSERVER_LOG_INFO(g_logger) << "idle";
        while (!stopping())
        {
            ljrserver::Fiber::YieldToHold();
        }
    }

} // namespace ljrserver
