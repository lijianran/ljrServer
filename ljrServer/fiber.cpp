
#include <atomic>

#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"

namespace ljrserver {

// system 日志
static Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

// 原子量 协程 id 只增
static std::atomic<uint64_t> s_fiber_id{0};
// 原子量 协程数目
static std::atomic<uint64_t> s_fiber_count{0};

// 线程局部变量 线程当前执行的线程
static thread_local Fiber *t_fiber = nullptr;
// 线程局部变量 线程的主协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

// 配置 栈大小 1m
static ConfigVar<uint32_t>::ptr g_fiber_static_size = Config::Lookup<uint32_t>(
    "fiber.stack_size", 128 * 1024, "fiber stack size");

/**
 * @brief 内存管理
 *
 */
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) { return malloc(size); }

    static void Dealloc(void *vp, size_t size) { return free(vp); }
};

// 别名，方便修改
using StackAllocator = MallocStackAllocator;

/**
 * @brief 不允许外部进行默认构造，设置为私有 private 函数
 *
 * 默认构造函数 实例化 main_fiber 主协程
 */
Fiber::Fiber() {
    // 设置协程状态
    m_state = EXEC;

    // 设置当前协程 t_fiber
    SetThis(this);

    // 获取 main_fiber 主协程
    if (getcontext(&m_context)) {
        LJRSERVER_ASSERT2(false, "getcontext");
    }

    // 协程数目 +1
    ++s_fiber_count;

    // main_fiber 主协程
    LJRSERVER_LOG_DEBUG(g_logger) << "Fiber::Fiber main"
                                  << " total=" << s_fiber_count;
}

/**
 * @brief 协程构造函数
 *
 * @param cb 协程执行函数
 * @param stacksize 协程栈大小 = 0
 * @param use_caller 是否使用 caller = false
 */
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb) {
    // 协程数
    ++s_fiber_count;

    // 栈大小
    m_stacksize = stacksize ? stacksize : g_fiber_static_size->getValue();

    // 申请栈内存
    m_stack = StackAllocator::Alloc(m_stacksize);

    // 获取当前线程执行的上下文
    if (getcontext(&m_context)) {
        LJRSERVER_ASSERT2(false, "getcontext");
    }

    // 后续执行的协程
    m_context.uc_link = nullptr;
    // 协程栈指针 sp
    m_context.uc_stack.ss_sp = m_stack;
    // 协程栈大小
    m_context.uc_stack.ss_size = m_stacksize;

    if (!use_caller) {
        // 使用调度器 Scheduler
        makecontext(&m_context, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_context, &Fiber::CallerMainFunc, 0);
    }

    LJRSERVER_LOG_DEBUG(g_logger)
        << "Fiber::Fiber id = " << m_id << " total=" << s_fiber_count;
}

/**
 * @brief 协程析构函数
 *
 */
Fiber::~Fiber() {
    // 协程数 -1
    --s_fiber_count;
    if (m_stack) {
        // 栈的内存没有销毁
        LJRSERVER_ASSERT(m_state == TERM || m_state == EXCEPT ||
                         m_state == INIT);

        // 销毁栈内存
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        // 栈内存已经销毁
        LJRSERVER_ASSERT(!m_cb);            // 函数执行完毕
        LJRSERVER_ASSERT(m_state == EXEC);  // 还在执行状态

        Fiber *cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }

    LJRSERVER_LOG_DEBUG(g_logger)
        << "Fiber::~Fiber id=" << m_id << " total=" << s_fiber_count;
}

/**
 * @brief 重置协程函数，并重置状态 INIT、TERM、EXCEPT
 *
 * @param cb 协程执行函数
 */
void Fiber::reset(std::function<void()> cb) {
    LJRSERVER_ASSERT(m_stack);
    LJRSERVER_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);

    // 重设协程执行函数
    m_cb = cb;

    // 获取线程当前执行的协程
    if (getcontext(&m_context)) {
        LJRSERVER_ASSERT2(false, "getcontext");
    }

    // 设置上下文属性
    m_context.uc_link = nullptr;
    m_context.uc_stack.ss_sp = m_stack;
    m_context.uc_stack.ss_size = m_stacksize;

    // 重设
    makecontext(&m_context, &Fiber::MainFunc, 0);

    // 协程状态
    m_state = INIT;
}

/**
 * @brief 调度器 切换到当前协程执行
 *
 */
void Fiber::swapIn() {
    // 设置线程当前执行的协程
    SetThis(this);

    // 当前不是执行状态
    LJRSERVER_ASSERT(m_state != EXEC);

    // 设置执行状态
    m_state = EXEC;

    // if (swapcontext(&t_threadFiber->m_context, &m_context))
    // Scheduler: main_fiber -> fiber
    if (swapcontext(&Scheduler::GetMainFiber()->m_context, &m_context)) {
        LJRSERVER_ASSERT2(false, "swapcontext swapin");
    }
}

/**
 * @brief 调度器 切换到后台执行
 *
 */
void Fiber::swapOut() {
    // SetThis(t_threadFiber.get());

    // 设置线程当前执行调度器的 main_fiber 主协程
    SetThis(Scheduler::GetMainFiber());

    // if (swapcontext(&m_context, &t_threadFiber->m_context))
    // Scheduler: fiber -> main_fiber
    if (swapcontext(&m_context, &Scheduler::GetMainFiber()->m_context)) {
        LJRSERVER_ASSERT2(false, "swapcontext swapout");
    }
}

/**
 * @brief call into fiber 进入协程
 *
 */
void Fiber::call() {
    // 设置线程当前执行的协程
    SetThis(this);

    // 执行状态
    m_state = EXEC;

    // main_fiber -> fiber
    if (swapcontext(&t_threadFiber->m_context, &m_context)) {
        LJRSERVER_ASSERT2(false, "swapcontext call");
    }
}

/**
 * @brief back to main_fiber 回到线程的主协程
 *
 */
void Fiber::back() {
    // 设置线程当前执行 main_fiber 主协程
    SetThis(t_threadFiber.get());

    // fiber -> main_fiber
    if (swapcontext(&m_context, &t_threadFiber->m_context)) {
        LJRSERVER_ASSERT2(false, "swapcontext back");
    }
}

/********************************
Fiber 类的静态成员函数
********************************/

/**
 * @brief 获取协程 id
 *
 * @return uint64_t
 */
uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

/**
 * @brief 设置线程当前执行的协程
 *
 * @param f 协程 Fiber 实例对象
 */
void Fiber::SetThis(Fiber *f) { t_fiber = f; }

/**
 * @brief 得到线程当前执行的协程
 *
 * 没有协程会构造 main_fiber 主协程
 *
 * @return Fiber::ptr
 */
Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        // 返回线程当前执行的协程
        return t_fiber->shared_from_this();
    }

    // 当前线程刚启动，一个协程都没有，通过默认构造函数，创建 main_fiber 主协程
    Fiber::ptr main_fiber(new Fiber);

    // 当前线程执行的协程是 main_fiber 主协程
    LJRSERVER_ASSERT(t_fiber == main_fiber.get());

    // 设置当前线程执行的 main_fiber 主协程
    t_threadFiber = main_fiber;

    // 返回 main_fiber 主协程
    return t_fiber->shared_from_this();
}

/**
 * @brief 调度器 协程切换到后台，并设置为 READY 状态
 *
 */
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

/**
 * @brief 调度器 协程切换到后台，并设置为 HOLD 状态
 *
 */
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

/**
 * @brief 获取总协程数
 *
 * @return uint64_t
 */
uint64_t Fiber::TotalFibers() { return s_fiber_count; }

/**
 * @brief 协程启动函数
 *
 * 使用 Scheduler
 */
void Fiber::MainFunc() {
    // 获取当前协程对象
    Fiber::ptr cur = GetThis();
    LJRSERVER_ASSERT(cur);

    try {
        // 执行协程函数
        cur->m_cb();
        // 执行完毕
        cur->m_cb = nullptr;
        // 设置状态终止
        cur->m_state = TERM;

    } catch (const std::exception &e) {
        // std::cerr << e.what() << '\n';
        // 出现异常
        cur->m_state = EXCEPT;
        LJRSERVER_LOG_ERROR(g_logger)
            << "Fiber Except: " << e.what() << " fiber_id = " << cur->m_id
            << std::endl
            << ljrserver::BacktraceToString();

    } catch (...) {
        // 其他异常
        cur->m_state = EXCEPT;
        LJRSERVER_LOG_ERROR(g_logger)
            << "Fiber Except"
            << " fiber_id = " << cur->m_id << std::endl
            << ljrserver::BacktraceToString();
    }

    // 取出裸指针
    auto raw_ptr = cur.get();

    // 确保智能指针析构
    cur.reset();

    // 回到调度器 Scheduler 的主协程 mian_fiber
    raw_ptr->swapOut();

    LJRSERVER_ASSERT2(
        false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));
}

/**
 * @brief 协程启动函数
 *
 * 使用当前 caller 线程
 */
void Fiber::CallerMainFunc() {
    // 获取当前协程对象
    Fiber::ptr cur = GetThis();
    LJRSERVER_ASSERT(cur);

    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;

    } catch (const std::exception &e) {
        // std::cerr << e.what() << '\n';
        cur->m_state = EXCEPT;
        LJRSERVER_LOG_ERROR(g_logger)
            << "Fiber Except: " << e.what() << " fiber_id = " << cur->m_id
            << std::endl
            << ljrserver::BacktraceToString();

    } catch (...) {
        cur->m_state = EXCEPT;
        LJRSERVER_LOG_ERROR(g_logger)
            << "Fiber Except"
            << " fiber_id = " << cur->m_id << std::endl
            << ljrserver::BacktraceToString();
    }

    // 确保智能指针析构
    auto raw_ptr = cur.get();
    cur.reset();

    // 回到主协程 mian_fiber
    raw_ptr->back();

    LJRSERVER_ASSERT2(
        false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));
}

}  // namespace ljrserver
