
#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace ljrserver {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

// 线程局部变量 当前线程的协程调度器 每个线程都一样指向调度器实例
static thread_local Scheduler *t_scheduler = nullptr;
// 线程局部变量 每个线程都指向当前线程执行 Scheduler::run() 的主协程
static thread_local Fiber *t_scheduler_fiber = nullptr;

/**
 * @brief 调度器构造函数
 *
 * @param threads 线程池大小
 * @param use_caller 是否使用 caller 的线程 [= true]
 * @param name 调度器名称
 */
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
    : m_name(name) {
    // 线程数大于 0
    LJRSERVER_ASSERT(threads > 0);

    // 使用 caller 的线程
    if (use_caller) {
        // 设置调度器线程的名称
        ljrserver::Thread::SetName(m_name);

        // 创建 caller 线程的主协程 main_fiber
        ljrserver::Fiber::GetThis();

        // 线程数 -1
        --threads;

        // 防止已经有调度器
        LJRSERVER_ASSERT(GetThis() == nullptr);

        // 设置当前线程的协程调度器
        t_scheduler = this;

        // 调度器协程 -> Scheduler::run
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        // static 函数 run 需要 std::bind 到 this 实例

        // 设置调度器线程的名称
        // ljrserver::Thread::SetName(m_name);

        // 调度器当前执行的协程 即执行调度器的协程
        t_scheduler_fiber = m_rootFiber.get();

        // 设置调度器的线程 id
        m_rootThread = ljrserver::GetThreadId();

        // 加入线程池 id
        m_threadIds.push_back(m_rootThread);

    } else {
        // 不使用 caller
        m_rootThread = -1;
    }

    // 线程个数
    m_threadCount = threads;
}

/**
 * @brief 调度器析构函数
 *
 */
Scheduler::~Scheduler() {
    // 调度器已经停止
    LJRSERVER_ASSERT(m_stopping);

    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

/**
 * @brief 获取当前线程指向的调度器实例对象
 *
 * @return Scheduler*
 */
Scheduler *Scheduler::GetThis() { return t_scheduler; }

/**
 * @brief 获取当前线程执行调度的主协程
 *
 * @return Fiber*
 */
Fiber *Scheduler::GetMainFiber() { return t_scheduler_fiber; }

/**
 * @brief 启动线程池 开启调度
 *
 */
void Scheduler::start() {
    // 上锁
    MutexType::Lock lock(m_mutex);

    if (!m_stopping) {
        // 正在执行，不要重复 start
        return;
    }
    m_stopping = false;

    // 线程池不为空
    LJRSERVER_ASSERT(m_threads.empty());

    // resize 线程池大小
    m_threads.resize(m_threadCount);

    // 线程池
    for (size_t i = 0; i < m_threadCount; ++i) {
        // 开启线程
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                      m_name + "_" + std::to_string(i)));

        // Thread 构造函数中加了信号量，能确保
        // 线程 id 会初始化，这里能获取正确的线程 id
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

    // if (m_rootFiber)
    // {
    //     m_rootFiber->call();
    // }
}

/**
 * @brief 停止调度
 *
 */
void Scheduler::stop() {
    m_autoStop = true;

    // 只有调度器协程
    if (m_rootFiber && m_threadCount == 0 &&
        (m_rootFiber->getState() == Fiber::TERM ||
         m_rootFiber->getState() == Fiber::INIT)) {
        // 调度器关闭
        LJRSERVER_LOG_INFO(g_logger) << this << " stopped";

        m_stopping = true;

        // 没有其他任务了
        if (stopping()) {
            return;
        }
    }

    // bool exit_on_this_fiber = false;
    if (m_rootThread != -1) {
        LJRSERVER_ASSERT(GetThis() == this);
    } else {
        LJRSERVER_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }

    if (m_rootFiber) {
        // while (!stopping())
        // {
        //     if (m_rootFiber->getState() == Fiber::TERM ||
        //     m_rootFiber->getState() == Fiber::EXCEPT)
        //     {
        //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this),
        //         0, true)); LJRSERVER_LOG_INFO(g_logger) << " root fiber is
        //         term, reset"; t_scheduler_fiber = m_rootFiber.get();
        //     }
        //     m_rootFiber->call();
        // }
        if (!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;

    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for (auto &i : thrs) {
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

/**
 * @brief 设置当前线程指向的调度器实例对象
 *
 */
void Scheduler::setThis() { t_scheduler = this; }

/**
 * @brief 调度函数
 *
 */
void Scheduler::run() {
    LJRSERVER_LOG_DEBUG(g_logger) << "new thread run";

    // 当前线程是否开启 HOOK
    set_hook_enable(false);

    // 设置当前线程的协程调度器 t_scheduler
    setThis();

    // 如果当前线程不是调度器线程
    if (ljrserver::GetThreadId() != m_rootThread) {
        // 设置当前线程执行的协程 t_scheduler_fiber
        t_scheduler_fiber = Fiber::GetThis().get();
        // Fiber::GetThis() 会创建当前线程的 main_fiber 主协程
    }

    // 当前线程的 idle 协程
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    // 回调函数协程
    Fiber::ptr cb_fiber;
    // 协程任务: 协程/回调函数
    FiberAndThread ft;

    // 协程调度循环
    while (true) {
        ft.reset();

        bool tickle_me = false;
        bool is_active = false;

        {
            // 在协程队列中找需要执行的协程任务，需要枷锁
            MutexType::Lock lock(m_mutex);

            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                // 有指定线程 id，但不是当前线程，continue，同时提醒其他线程处理
                if (it->thread != -1 &&
                    it->thread != ljrserver::GetThreadId()) {
                    ++it;
                    // 提醒其他线程
                    tickle_me = true;
                    continue;
                }

                LJRSERVER_ASSERT(it->fiber || it->cb);
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    // 已经开始执行的协程，continue
                    ++it;
                    continue;
                }

                // 取出协程任务
                ft = *it;
                m_fibers.erase(it);

                // 工作线程 +1
                ++m_activeThreadCount;

                // 找到要执行的任务
                is_active = true;
                break;
            }
        }

        // 需要提醒其他线程
        if (tickle_me) {
            tickle();
        }

        // 如果是协程形式且状态不是终止和意外
        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM &&
                         ft.fiber->getState() != Fiber::EXCEPT)) {
            // 执行协程
            ft.fiber->swapIn();
            // 执行完毕 工作线程 -1
            --m_activeThreadCount;

            // 协程出来后
            if (ft.fiber->getState() == Fiber::READY) {
                // 若状态位 ready，则继续进协程队列等待处理
                schedule(ft.fiber);

            } else if (ft.fiber->getState() != Fiber::TERM &&
                       ft.fiber->getState() != Fiber::EXCEPT) {
                // 如果协程不是终止或意外，则设为 hold 挂起状态
                ft.fiber->m_state = Fiber::HOLD;
            }

            // 清空当前任务
            ft.reset();

        } else if (ft.cb) {
            // 如果是回调函数形式
            // 判断回调函数协程是否已经有内存
            if (cb_fiber) {
                // 重置
                cb_fiber->reset(ft.cb);
            } else {
                // 新建
                cb_fiber.reset(new Fiber(ft.cb));
            }

            // 清空当前任务
            ft.reset();

            // 执行协程任务
            cb_fiber->swapIn();
            // 执行完毕 工作线程 -1
            --m_activeThreadCount;

            // 协程出来后
            if (cb_fiber->getState() == Fiber::READY) {
                // 状态是 ready，则进入协程队列
                schedule(cb_fiber);
                // 清空 cb_fiber
                cb_fiber.reset();

            } else if (cb_fiber->getState() == Fiber::EXCEPT ||
                       cb_fiber->getState() == Fiber::TERM) {
                // 终止和意外的情况，重置为空
                cb_fiber->reset(nullptr);

            } else  // if (cb_fiber->getState() != Fiber::TERM)
            {
                // 其他状态，设为 hold 挂起
                cb_fiber->m_state = Fiber::HOLD;
                // 清空 cb_fiber
                cb_fiber.reset();
            }

        } else {
            // 其他形式
            if (is_active) {
                // 确定有任务
                --m_activeThreadCount;
                // 工作线程 -1 继续找任务
                continue;
            }

            // 协程任务队列中没有待处理的任务，则使用 idle 协程占用 cpu
            if (idle_fiber->getState() == Fiber::TERM) {
                LJRSERVER_LOG_INFO(g_logger) << "idle fiber terminate";
                break;
            }

            // 进入 idle_fiber
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;

            if (idle_fiber->getState() != Fiber::TERM &&
                idle_fiber->getState() != Fiber::EXCEPT) {
                // 设置 idle_fiber 状态为 hold
                idle_fiber->m_state = Fiber::HOLD;
                // 调度器作为友元类 可以直接操作 fiber 对象的成员变量
            }
        }
    }
}

/**
 * @brief 提醒其他线程 虚函数
 *
 */
void Scheduler::tickle() { LJRSERVER_LOG_INFO(g_logger) << "tickle"; }

/**
 * @brief 是否正在停止 虚函数
 *
 * @return true
 * @return false
 */
bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);

    return m_autoStop && m_stopping && m_fibers.empty() &&
           m_activeThreadCount == 0;
}

/**
 * @brief 闲置协程 虚函数 由子类各自实现
 *
 */
void Scheduler::idle() {
    LJRSERVER_LOG_INFO(g_logger) << "idle";
    while (!stopping()) {
        ljrserver::Fiber::YieldToHold();
    }
}

}  // namespace ljrserver
