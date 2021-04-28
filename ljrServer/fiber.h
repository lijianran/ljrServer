
#ifndef __LJRSERVER_FIBER_H__
#define __LJRSERVER_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace ljrserver
{

    class Scheduler;
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
        friend class Scheduler;

    public:
        typedef std::shared_ptr<Fiber> ptr;

        enum State
        {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT
        };

    private:
        // 不允许默认构造，设置为私有private函数
        Fiber();

    public:
        Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

        ~Fiber();

        // 重置协程函数，并重置状态 INIT、TERM
        void reset(std::function<void()> cb);

        // 切换到当前协程执行
        void swapIn();

        // 切换到后台执行
        void swapOut();

        void call();

        void back();

        uint64_t getId() const { return m_id; }

        State getState() const { return m_state; }

        // void setState(const State &s) { m_state = s; }

    public:
        // 获取协程id
        static uint64_t GetFiberId();

        // 设置当前执行的协程
        static void SetThis(Fiber *f);

        // 返回当前执行的协程
        static Fiber::ptr GetThis();

        // 协程切换到后台，并设置为READY状态
        static void YieldToReady();

        // 协程切换到后台，并设置为HOLD状态
        static void YieldToHold();

        // 总协程数
        static uint64_t TotalFibers();

        static void MainFunc();
        static void CallerMainFunc();


    private:
        // 协程id
        uint64_t m_id = 0;
        // 协程栈大小
        uint32_t m_stacksize = 0;
        // 协程状态
        State m_state = INIT;
        // ucontext_t
        ucontext_t m_context;
        // 协程栈的内存空间
        void *m_stack = nullptr;
        // 协程方法
        std::function<void()> m_cb;
    };

} // namespace ljrserver

#endif // __LJRSERVER_FIBER_H__