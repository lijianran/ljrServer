
#include "iomanager.h"
#include "log.h"
#include "macro.h"

// pipe
#include <unistd.h>
// fcntl
#include <fcntl.h>
// epoll
#include <sys/epoll.h>
// error
#include <errno.h>
// string
#include <string.h>

namespace ljrserver {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/**
 * @brief 获得事件上下文
 *
 * @param event 事件
 * @return EventContext& 事件上下文
 */
IOManager::FdContext::EventContext &IOManager::FdContext::getContext(
    Event event) {
    // epoll 事件
    // 无事件 NONE = 0x0,
    // 读事件 (EPOLLIN)  READ  = 0x1,
    // 写事件 (EPOLLOUT) WRITE = 0x4,
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            LJRSERVER_ASSERT2(false, "getContext");
    }
}

/**
 * @brief 重置事件上下文
 *
 * @param ctx 事件上下文
 */
void IOManager::FdContext::resetContext(EventContext &ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

/**
 * @brief 触发事件
 *
 * @param event epoll 事件
 */
void IOManager::FdContext::triggerEvent(Event event) {
    // 获取事件
    LJRSERVER_ASSERT(events & event);
    events = (Event)(events & ~event);

    // 获取事件上下文
    EventContext &ctx = getContext(event);
    if (ctx.cb) {
        // 调度函数任务
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        // 调度协程任务
        ctx.scheduler->schedule(&ctx.fiber);
    }

    ctx.scheduler = nullptr;
}

/**
 * @brief IO 协程调度器的构造函数
 *
 * @param threads 线程数
 * @param use_caller 是否使用 caller 线程 [= true]
 * @param name 调度器名称
 */
IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
    // 创建 epoll 实例
    m_epollfd = epoll_create(5000);
    LJRSERVER_ASSERT(m_epollfd > 0);

    // 创建管道
    int rt = pipe(m_tickleFds);
    LJRSERVER_ASSERT(!rt);

    // epoll 事件
    epoll_event event;
    // 清零
    memset(&event, 0, sizeof(epoll_event));
    // ET 模式 边缘触发 一次事件只会通知一次，不处理也不会再通知
    event.events = EPOLLIN | EPOLLET;
    // 事件文件句柄
    event.data.fd = m_tickleFds[0];

    // 设置 pipe 非阻塞
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    LJRSERVER_ASSERT(!rt);

    // tickle
    rt = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    LJRSERVER_ASSERT(!rt);

    // 句柄数组 resize
    // m_fdContexts.resize(64);
    contextResize(32);

    // Scheduler::start(); 开始调度
    start();
}

/**
 * @brief IO 协程调度器的析构函数
 *
 */
IOManager::~IOManager() {
    // Scheduler::stop(); 停止调度
    stop();

    // 关闭句柄
    close(m_epollfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    // 清理内存
    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

/**
 * @brief 句柄上下文数组大小调整
 *
 * @param size 数组大小
 */
void IOManager::contextResize(size_t size) {
    // 调整 vector 大小
    m_fdContexts.resize(size);

    // 分配句柄
    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

/**
 * @brief 添加事件
 *
 * @param fd 事件句柄
 * @param event 事件类型
 * @param cb 事件函数 [= nullptr]
 * @return int 0 success
 */
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    // 创建句柄事件上下文
    FdContext *fd_ctx = nullptr;
    // 上读锁
    RWMutexType::ReadLock lock(m_mutex);

    if ((int)m_fdContexts.size() > fd) {
        // 直接取句柄上下文
        fd_ctx = m_fdContexts[fd];
        // 解读锁
        lock.unlock();
    } else {
        // 解读锁
        lock.unlock();

        // 上写锁
        RWMutexType::WriteLock lock2(m_mutex);
        // 1.5 倍扩容句柄数组
        contextResize(fd * 1.5);
        // 读取新分配的句柄
        fd_ctx = m_fdContexts[fd];
    }

    // 句柄上下文互斥锁
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);

    // 已经有该事件
    if (fd_ctx->events & event) {
        LJRSERVER_LOG_ERROR(g_logger)
            << "addEvent assert fd = " << fd << " event = " << event
            << " fd_ctx.events = " << fd_ctx->events;

        // assert 终止
        LJRSERVER_ASSERT(!(fd_ctx->events & event));
    }

    // 操作: 当前有事件则 mod 修改 无事件则 add 增加
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

    // epoll 事件
    epoll_event epevent;
    // 边缘触发
    epevent.events = EPOLLET | fd_ctx->events | event;
    // 指向事件处理上下文
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epollfd, op, fd, &epevent);
    if (rt) {
        LJRSERVER_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epollfd << ", " << op << ", " << fd << ", "
            << epevent.events << "): " << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return -1;
    }

    // 任务 +1
    ++m_pendingEventCount;
    // 设置当前事件
    fd_ctx->events = (Event)(fd_ctx->events | event);
    // 获取事件上下文
    FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
    // scheduler fiber cb 都为空
    LJRSERVER_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    // 设置调度器
    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        // 有任务函数
        event_ctx.cb.swap(cb);
    } else {
        // 设置当前协程作为要调度的任务
        event_ctx.fiber = Fiber::GetThis();
        // 且该协程任务正在执行
        LJRSERVER_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }

    return 0;
}

/**
 * @brief 删除事件 不会触发事件
 *
 * @param fd 事件句柄
 * @param event 事件
 * @return true
 * @return false
 */
bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }

    // 取出要删除事件的上下文
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        // 没有该事件
        return false;
    }

    // 删除事件
    Event new_events = (Event)(fd_ctx->events & ~event);
    // 操作: 删除后还有事件则 mod 修改 无事件则 del 删除
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    // epoll 事件
    epoll_event epevent;
    // 边缘触发
    epevent.events = EPOLLET | new_events;
    // 事件上下文
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epollfd, op, fd, &epevent);
    if (rt) {
        LJRSERVER_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epollfd << ", " << op << ", " << fd << ", "
            << epevent.events << "): " << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return false;
    }

    // 触发事件
    // fd_ctx->triggerEvent(event);
    // 任务 -1
    // --m_pendingEventCount;
    // FdContext::EventContext &event_ctx = fd_ctx->getContext(event);

    // 任务 -1
    --m_pendingEventCount;
    // 设置上下文事件
    fd_ctx->events = new_events;
    // 获取事件上下文
    FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
    // 重置事件上下文
    fd_ctx->resetContext(event_ctx);

    return true;
}

/**
 * @brief 取消事件 如果事件存在则触发事件
 *
 * @param fd 事件句柄
 * @param event 事件
 * @return true
 * @return false
 */
bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }

    // 取出要取消事件的上下文
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        // 没有该事件
        return false;
    }

    // 取消事件
    Event new_events = (Event)(fd_ctx->events & ~event);
    // 操作: 取消后还有事件则 mod 修改 无事件则 del 删除
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    // epoll 事件
    epoll_event epevent;
    // 边缘触发
    epevent.events = EPOLLET | new_events;
    // 事件上下文
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epollfd, op, fd, &epevent);
    if (rt) {
        LJRSERVER_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epollfd << ", " << op << ", " << fd << ", "
            << epevent.events << "): " << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return false;
    }

    // 触发事件
    fd_ctx->triggerEvent(event);
    // 任务 -1
    --m_pendingEventCount;

    return true;
}

/**
 * @brief 取消句柄下所有事件 会执行事件
 *
 * @param fd 事件句柄
 * @return true
 * @return false
 */
bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }

    // 取出事件的上下文
    FdContext *fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!fd_ctx->events) {
        // 已经没有事件了
        return false;
    }

    // Event new_events = (Event)(fd_ctx->events & ~event);
    // int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    // 操作: del 删除事件
    int op = EPOLL_CTL_DEL;

    // epoll 事件
    epoll_event epevent;
    // epevent.events = EPOLLET | new_events;
    // 清空事件
    epevent.events = 0;
    // 事件上下文
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epollfd, op, fd, &epevent);
    if (rt) {
        LJRSERVER_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epollfd << ", " << op << ", " << fd << ", "
            << epevent.events << "): " << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return false;
    }

    // 还有读事件
    if (fd_ctx->events & READ) {
        // 触发事件
        fd_ctx->triggerEvent(READ);
        // 任务 -1
        --m_pendingEventCount;
    }
    // 还有写事件
    if (fd_ctx->events & WRITE) {
        // 触发事件
        fd_ctx->triggerEvent(WRITE);
        // 任务 -1
        --m_pendingEventCount;
    }

    // 事件执行完毕
    LJRSERVER_ASSERT(fd_ctx->events == 0);
    return true;
}

/**
 * @brief 获取当前的 IO 管理器
 * static
 *
 * @return IOManager*
 */
IOManager *IOManager::GetThis() {
    // 动态转换 Scheduler -> IOManager
    return dynamic_cast<IOManager *>(Scheduler::GetThis());
}

/**
 * @brief 虚函数的实现 唤醒
 *
 */
void IOManager::tickle() {
    if (!hasIdleThreads()) {
        // 没有闲置线程则不 tickle
        return;
    }

    // 使用 write 系统调用向 pipe 管道写入内容 唤醒相应进程
    int rt = write(m_tickleFds[1], "T", 1);

    // 成功返回长度
    LJRSERVER_ASSERT(rt == 1);
}

/**
 * @brief 虚函数的实现 是否正在停止
 *
 * @return true
 * @return false
 */
bool IOManager::stopping() {
    // return m_pendingEventCount == 0 && Scheduler::stopping();
    uint64_t timeout = 0;
    return stopping(timeout);
}

/**
 * @brief 虚函数的实现 是否正在停止
 *
 * 定时器实现
 *
 * @param timeout 下一个定时任务执行还要多久
 * @return true
 * @return false
 */
bool IOManager::stopping(uint64_t &timeout) {
    // 下一个定时任务执行还要多久
    timeout = getNextTimer();

    // 没有定时任务且要处理的事件个数为 0 且调度器 Scheduler::stopping();
    return timeout == ~0ull && m_pendingEventCount == 0 &&
           Scheduler::stopping();
}

/**
 * @brief 虚函数的实现 idle_fiber
 *
 * 负责没有事件时占用 CPU 等待事件
 *
 */
void IOManager::idle() {
    // idle_fiber

    // 不在栈上创建太大的数组，用 new 申请堆内存
    epoll_event *events = new epoll_event[64]();

    // 通过智能指针，释放数组内存
    std::shared_ptr<epoll_event> shared_events(
        events, [](epoll_event *ptr) { delete[] ptr; });

    while (true) {
        // if (stopping())
        // {
        //     LJRSERVER_LOG_INFO(g_logger) << "name = " << getName()
        //                                  << " idle stopping exit";
        //     break;
        // }

        // 下一个定时任务执行还要多久
        uint64_t next_timeout = 0;
        if (stopping(next_timeout)) {
            // next_timeout = getNextTimer();
            // if (next_timeout == ~0ull)
            // {
            LJRSERVER_LOG_INFO(g_logger)
                << "name=" << getName() << " idle_fiber stopping exit";
            break;
            // }
        }

        // epoll_wait 返回值 -1 代表有错误 >0 为事件个数
        int rt = 0;
        // 循环等待 IO 事件
        do {
            // epoll_wait __timeout 等待时间
            static const int MAX_TIMEOUT = 5000;

            // 是否有定时任务
            if (next_timeout != ~0ull) {
                // 下一个定时任务执行时间是否大于 5s 是否需要在下一次任务前唤醒
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT
                                                               : next_timeout;
            } else {
                // 没有定时任务
                next_timeout = MAX_TIMEOUT;
            }

            // rt = epoll_wait(m_epollfd, events, 64, MAX_TIMEOUT);
            rt = epoll_wait(m_epollfd, events, 64, (int)next_timeout);

            if (rt < 0 && errno == EINTR) {
                // rt 为事件个数 rt = -1 并且中断产生了 EINTR
                // 即网络系统调用被 os 中断
            } else {
                // 有事件到来 开始处理
                break;
            }

        } while (true);

        // 列出要执行的定时任务
        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);

        // 是否有定时任务要执行
        if (!cbs.empty()) {
            // LJRSERVER_LOG_DEBUG(g_logger)
            //     << "on timer cbs.size = " << cbs.size();

            // 调度任务
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        // 处理句柄事件 rt 为事件个数
        for (int i = 0; i < rt; ++i) {
            // 取出 epoll 事件
            epoll_event &event = events[i];

            // tickle 唤醒事件
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy;
                while (read(m_tickleFds[0], &dummy, 1) == 1) {
                    // read 系统调用
                }
                // 结束 tickle 唤醒
                continue;
            }

            // 取出句柄事件上下文
            FdContext *fd_ctx = (FdContext *)event.data.ptr;
            // 上锁
            FdContext::MutexType::Lock lock(fd_ctx->mutex);

            // 有错误
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                // 读写全部开启
                event.events |= EPOLLIN | EPOLLOUT;
            }

            // 判断事件
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                // 读事件
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                // 写事件
                real_events |= WRITE;
            }

            // 没有句柄事件上下文要处理的事件
            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            // 剩下的事件
            int left_events = (fd_ctx->events & ~real_events);
            // 操作: 还有事件要处理则 mod 修改 无事件要处理则 del 删除
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            // 边缘触发
            event.events = EPOLLET | left_events;

            // 继续配置 epoll
            int rt2 = epoll_ctl(m_epollfd, op, fd_ctx->fd, &event);
            if (rt2) {
                // 返回值不为零 错误
                LJRSERVER_LOG_ERROR(g_logger)
                    << "epoll_ctl(" << m_epollfd << ", " << op << ", "
                    << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events
                    << "): " << rt2 << " (" << errno << ") (" << strerror(errno)
                    << ")";
                continue;
            }

            // 处理读事件
            if (real_events & READ) {
                // 调度处理
                fd_ctx->triggerEvent(READ);
                // 任务 -1
                --m_pendingEventCount;
            }
            // 处理写事件
            if (real_events & WRITE) {
                // 调度处理
                fd_ctx->triggerEvent(WRITE);
                // 任务 -1
                --m_pendingEventCount;
            }
        }

        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        // 回到调度主协程
        raw_ptr->swapOut();
    }
}

/**
 * @brief TimerManager 纯虚函数的实现
 *
 * 当有新的定时器插入到定时器的首部 执行该函数 tickle 唤醒线程执行任务
 *
 */
void IOManager::onTimerInsertedAtFront() {
    // tickle 线程
    tickle();
}

}  // namespace ljrserver
