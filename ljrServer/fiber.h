
#ifndef __LJRSERVER_FIBER_H__
#define __LJRSERVER_FIBER_H__

// 智能指针
#include <memory>
// 函数包装
#include <functional>
// 协程
#include <ucontext.h>
// 线程
// #include "thread.h"

namespace ljrserver {

class Scheduler;

/**
 * @brief Class 协程的封装实现
 *
 */
class Fiber : public std::enable_shared_from_this<Fiber> {
    // 调度器作为友元类 可以直接操作成员变量
    friend class Scheduler;

public:
    typedef std::shared_ptr<Fiber> ptr;

    /**
     * @brief 协程状态
     * INIT, HOLD, EXEC, TERM, READY, EXCEPT
     */
    enum State { INIT, HOLD, EXEC, TERM, READY, EXCEPT };

private:
    /**
     * @brief 不允许外部进行默认构造，设置为私有 private 函数
     *
     * 默认构造函数 实例化 main_fiber 主协程
     */
    Fiber();

public:
    /**
     * @brief 协程构造函数
     *
     * @param cb 协程执行函数
     * @param stacksize 协程栈大小 = 0
     * @param use_caller 是否使用 caller [= false]
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0,
          bool use_caller = false);

    /**
     * @brief 协程析构函数
     *
     */
    ~Fiber();

    /**
     * @brief 重置协程函数，并重置状态 INIT、TERM、EXCEPT
     *
     * @param cb 协程执行函数
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 调度器 切换到当前协程执行
     *
     */
    void swapIn();

    /**
     * @brief 调度器 切换到后台执行
     *
     */
    void swapOut();

    /**
     * @brief call into fiber 进入协程
     *
     */
    void call();

    /**
     * @brief back to main_fiber 回到线程的主协程
     *
     */
    void back();

    /**
     * @brief 获取协程 id
     *
     * @return uint64_t
     */
    uint64_t getId() const { return m_id; }

    /**
     * @brief 获取协程状态
     *
     * @return State
     */
    State getState() const { return m_state; }

    // void setState(const State &s) { m_state = s; }

public:
    // 获取协程 id
    static uint64_t GetFiberId();

    /**
     * @brief 设置线程当前执行的协程
     *
     * @param f 协程 Fiber 实例对象
     */
    static void SetThis(Fiber *f);

    /**
     * @brief 得到线程当前执行的协程
     *
     * 没有协程会构造 main_fiber 主协程
     *
     * @return Fiber::ptr
     */
    static Fiber::ptr GetThis();

    /**
     * @brief 调度器 协程切换到后台，并设置为 READY 状态
     *
     */
    static void YieldToReady();

    /**
     * @brief 调度器 协程切换到后台，并设置为 HOLD 状态
     *
     */
    static void YieldToHold();

    /**
     * @brief 获取总协程数
     *
     * @return uint64_t
     */
    static uint64_t TotalFibers();

    /**
     * @brief 协程启动函数
     *
     * 使用 Scheduler
     */
    static void MainFunc();

    /**
     * @brief 协程启动函数
     *
     * 使用当前 caller 线程
     */
    static void CallerMainFunc();

private:
    // 协程 id
    uint64_t m_id = 0;

    // 协程栈大小
    uint32_t m_stacksize = 0;

    // 协程状态
    State m_state = INIT;

    // ucontext_t 协程具柄
    ucontext_t m_context;

    // 协程栈的内存空间
    void *m_stack = nullptr;

    // 协程执行方法
    std::function<void()> m_cb;
};

}  // namespace ljrserver

#endif  // __LJRSERVER_FIBER_H__