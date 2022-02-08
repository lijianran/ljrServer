
#ifndef __LJRSERVER_SCHEDULER_H__
#define __LJRSERVER_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>

#include "thread.h"
#include "fiber.h"

namespace ljrserver {

/**
 * @brief Class 协程调度器
 *
 * 协程调度器，管理协程的调度，内部实现为一个线程池，
 * 支持协程在多线程中切换，也可以指定协程在固定的线程中执行。
 * 是一个 N-M 的协程调度模型，N 个线程，M 个协程。
 * 重复利用每一个线程。
 */
class Scheduler {
public:
    // 智能指针
    typedef std::shared_ptr<Scheduler> ptr;

    // 互斥锁
    typedef Mutex MutexType;

    /**
     * @brief 调度器构造函数
     *
     * @param threads 线程池大小
     * @param use_caller 是否使用 caller 线程 [= true]
     * @param name 调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true,
              const std::string &name = "");

    /**
     * @brief 调度器析构函数
     *
     * 基类析构函数设置为虚函数
     */
    virtual ~Scheduler();

    /**
     * @brief 获取调度器名称
     *
     * @return const std::string&
     */
    const std::string &getName() const { return m_name; }

    /**
     * @brief 获取当前线程指向的调度器实例对象
     *
     * @return Scheduler*
     */
    static Scheduler *GetThis();

    /**
     * @brief 获取当前线程执行调度的主协程
     *
     * @return Fiber*
     */
    static Fiber *GetMainFiber();

    /**
     * @brief 启动线程池 开启调度
     *
     */
    void start();

    /**
     * @brief 停止调度
     *
     */
    void stop();

    /**
     * @brief 调度模版函数
     *
     * @tparam FiberOrCb
     * @param fc 协程 Fiber 对象或者函数
     * @param thread 指定线程 id
     */
    template <class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;

        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if (need_tickle) {
            tickle();
        }
    }

    /**
     * @brief 调度模版函数
     *
     * @tparam InputIterator
     * @param begin
     * @param end
     */
    template <class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = scheduleNoLock(&(*begin), -1) || need_tickle;
                ++begin;
            }
        }

        if (need_tickle) {
            tickle();
        }
    }

protected:
    /**
     * @brief 提醒其他线程 虚函数
     *
     */
    virtual void tickle();

    /**
     * @brief 是否正在停止 虚函数
     *
     * @return true
     * @return false
     */
    virtual bool stopping();

    /**
     * @brief 闲置协程 虚函数 由子类继承后各自实现
     *
     */
    virtual void idle();

    /**
     * @brief 调度函数
     *
     */
    void run();

    /**
     * @brief 设置当前线程指向的调度器实例对象
     *
     */
    void setThis();

    /**
     * @brief 是否有闲置的线程
     *
     * @return true
     * @return false
     */
    bool hasIdleThreads() { return m_idleThreadCount > 0; }

private:
    template <class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:
    /**
     * @brief 线程/协程 结构体
     *
     */
    struct FiberAndThread {
        // 协程实例
        Fiber::ptr fiber;
        // 执行函数
        std::function<void()> cb;
        // 线程 id
        int thread;

        /**
         * @brief 构造函数
         *
         * @param f
         * @param thr
         */
        FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}

        FiberAndThread(Fiber::ptr *f, int thr) : thread(thr) { fiber.swap(*f); }

        FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {}

        FiberAndThread(std::function<void()> *f, int thr) : thread(thr) {
            cb.swap(*f);
        }

        FiberAndThread() : thread(-1) {}

        /**
         * @brief 重置
         *
         */
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

protected:
    // 线程池 id
    std::vector<int> m_threadIds;

    // 线程数
    size_t m_threadCount = 0;

    // 活动线程数量
    std::atomic<size_t> m_activeThreadCount = {0};

    // 闲置线程数量
    std::atomic<size_t> m_idleThreadCount = {0};

    // 是否已经停止
    bool m_stopping = true;

    // 是否自动停止
    bool m_autoStop = false;

    // 调度器线程 id
    int m_rootThread = 0;

private:
    // 互斥量
    MutexType m_mutex;

    // 线程池
    std::vector<Thread::ptr> m_threads;

    // 协程队列
    std::list<FiberAndThread> m_fibers;

    // 调度器协程
    Fiber::ptr m_rootFiber;

    // 调度器名称
    std::string m_name;
};

}  // namespace ljrserver

#endif  // __LJRSERVER_SCHEDULER_H__