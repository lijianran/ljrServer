
#ifndef __LJRSERVER_SCHEDULER_H__
#define __LJRSERVER_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>

#include "thread.h"
#include "fiber.h"

namespace ljrserver
{

    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;

        Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");

        virtual ~Scheduler();

        const std::string &getName() const { return m_name; }

        // 获取当前调度器
        static Scheduler *GetThis();

        // 获取主协程
        static Fiber *GetMainFiber();

        // 启动线程池
        void start();

        // 停止
        void stop();

        template <class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1)
        {
            bool need_tickle = false;

            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }

            if (need_tickle)
            {
                tickle();
            }
        }

        template <class InputIterator>
        void schedule(InputIterator begin, InputIterator end)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end)
                {
                    need_tickle = scheduleNoLock(&(*begin), -1) || need_tickle;
                    ++begin;
                }
            }

            if (need_tickle)
            {
                tickle();
            }
        }

    protected:
        virtual void tickle();

        virtual bool stopping();

        virtual void idle();

        void run();

        void setThis();

        bool hasIdleThreads() { return m_idleThreadCount > 0; }

    private:
        template <class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread)
        {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            if (ft.fiber || ft.cb)
            {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        struct FiberAndThread
        {
            Fiber::ptr fiber;
            std::function<void()> cb;
            int thread;

            FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr)
            {
            }

            FiberAndThread(Fiber::ptr *f, int thr) : thread(thr)
            {
                fiber.swap(*f);
            }

            FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr)
            {
            }

            FiberAndThread(std::function<void()> *f, int thr) : thread(thr)
            {
                cb.swap(*f);
            }

            FiberAndThread() : thread(-1)
            {
            }

            void reset()
            {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };

    protected:
        std::vector<int> m_threadIds;
        // 线程数
        size_t m_threadCount = 0;
        std::atomic<size_t> m_activeThreadCount = {0};
        std::atomic<size_t> m_idleThreadCount = {0};
        bool m_stopping = true;
        bool m_autoStop = false;
        // 线程id
        int m_rootThread = 0;

    private:
        // 互斥量
        MutexType m_mutex;
        // 线程池
        std::vector<Thread::ptr> m_threads;
        // 协程队列
        std::list<FiberAndThread> m_fibers;
        // 主协程
        Fiber::ptr m_rootFiber;
        // 名称
        std::string m_name;
    };

} // namespace ljrserver

#endif // __LJRSERVER_SCHEDULER_H__