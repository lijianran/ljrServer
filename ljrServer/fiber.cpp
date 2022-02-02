
#include <atomic>

#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"

namespace ljrserver {

// 日志
static Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

// 原子量
static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

// 线程局部变量
static thread_local Fiber *t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

// 配置 栈大小1m
static ConfigVar<uint32_t>::ptr g_fiber_static_size = Config::Lookup<uint32_t>(
    "fiber.stack_size", 128 * 1024, "fiber stack size");

// 内存管理
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) { return malloc(size); }

    static void Dealloc(void *vp, size_t size) { return free(vp); }
};

// 别名，方便修改
using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
    // main 协程
    m_state = EXEC;
    SetThis(this);

    if (getcontext(&m_context)) {
        LJRSERVER_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;

    LJRSERVER_LOG_DEBUG(g_logger) << "Fiber::Fiber main"
                                  << " total=" << s_fiber_count;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_static_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if (getcontext(&m_context)) {
        LJRSERVER_ASSERT2(false, "getcontext");
    }

    m_context.uc_link = nullptr;
    m_context.uc_stack.ss_sp = m_stack;
    m_context.uc_stack.ss_size = m_stacksize;

    if (!use_caller) {
        makecontext(&m_context, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_context, &Fiber::CallerMainFunc, 0);
    }

    LJRSERVER_LOG_DEBUG(g_logger)
        << "Fiber::Fiber id = " << m_id << " total=" << s_fiber_count;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if (m_stack) {
        LJRSERVER_ASSERT(m_state == TERM || m_state == EXCEPT ||
                         m_state == INIT);

        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        LJRSERVER_ASSERT(!m_cb);
        LJRSERVER_ASSERT(m_state == EXEC);

        Fiber *cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }

    LJRSERVER_LOG_DEBUG(g_logger)
        << "Fiber::~Fiber id=" << m_id << " total=" << s_fiber_count;
}

// 重置协程函数，并重置状态 INIT、TERM、EXCEPT
void Fiber::reset(std::function<void()> cb) {
    LJRSERVER_ASSERT(m_stack);
    LJRSERVER_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);

    m_cb = cb;
    if (getcontext(&m_context)) {
        LJRSERVER_ASSERT2(false, "getcontext");
    }

    m_context.uc_link = nullptr;
    m_context.uc_stack.ss_sp = m_stack;
    m_context.uc_stack.ss_size = m_stacksize;

    makecontext(&m_context, &Fiber::MainFunc, 0);
    m_state = INIT;
}

// 切换到当前协程执行
void Fiber::swapIn() {
    SetThis(this);
    LJRSERVER_ASSERT(m_state != EXEC);
    m_state = EXEC;

    // if (swapcontext(&t_threadFiber->m_context, &m_context))
    if (swapcontext(&Scheduler::GetMainFiber()->m_context, &m_context)) {
        LJRSERVER_ASSERT2(false, "swapcontext swapin");
    }
}

// 切换到后台执行
void Fiber::swapOut() {
    // SetThis(t_threadFiber.get());
    SetThis(Scheduler::GetMainFiber());

    // if (swapcontext(&m_context, &t_threadFiber->m_context))
    if (swapcontext(&m_context, &Scheduler::GetMainFiber()->m_context)) {
        LJRSERVER_ASSERT2(false, "swapcontext swapout");
    }
}

void Fiber::call() {
    SetThis(this);

    m_state = EXEC;

    if (swapcontext(&t_threadFiber->m_context, &m_context)) {
        LJRSERVER_ASSERT2(false, "swapcontext call");
    }
}

void Fiber::back() {
    SetThis(t_threadFiber.get());

    if (swapcontext(&m_context, &t_threadFiber->m_context)) {
        LJRSERVER_ASSERT2(false, "swapcontext back");
    }
}

/********************************
Fiber 类的静态成员函数
********************************/
// 获取协程id
uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

// 设置当前执行的协程
void Fiber::SetThis(Fiber *f) { t_fiber = f; }

// 得到当前执行的协程
Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }

    // 当前线程刚启动，一个协程都没有，通过默认构造函数，创建 main_fiber 主协程
    Fiber::ptr main_fiber(new Fiber);
    LJRSERVER_ASSERT(t_fiber == main_fiber.get());

    t_threadFiber = main_fiber;

    return t_fiber->shared_from_this();
}

// 协程切换到后台，并设置为 READY 状态
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

// 协程切换到后台，并设置为 HOLD 状态
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

// 总协程数
uint64_t Fiber::TotalFibers() { return s_fiber_count; }

void Fiber::MainFunc() {
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
    raw_ptr->swapOut();

    LJRSERVER_ASSERT2(
        false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));
}

// back caller
void Fiber::CallerMainFunc() {
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
