
#include "iomanager.h"
#include "log.h"
#include "macro.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>

namespace ljrserver
{

    static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(Event event)
    {
        switch (event)
        {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            LJRSERVER_ASSERT2(false, "getContext");
        }
    }

    void IOManager::FdContext::resetContext(EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(Event event)
    {
        LJRSERVER_ASSERT(events & event);
        events = (Event)(events & ~event);
        EventContext &ctx = getContext(event);
        if (ctx.cb)
        {
            ctx.scheduler->schedule(&ctx.cb);
        }
        else
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }

        ctx.scheduler = nullptr;
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
    {
        m_epollfd = epoll_create(5000);
        LJRSERVER_ASSERT(m_epollfd > 0);

        int rt = pipe(m_tickleFds);
        LJRSERVER_ASSERT(!rt);

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        // ET 模式 边缘触发 一次事件只会通知一次，不处理也不会再通知
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];

        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        LJRSERVER_ASSERT(!rt);

        rt = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        LJRSERVER_ASSERT(!rt);

        // m_fdContexts.resize(64);
        contextResize(32);

        start();
    }

    IOManager::~IOManager()
    {
        stop();
        close(m_epollfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i])
            {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    // 添加事件，1 success, 2 retry, -1 error
    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd)
        {
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        }
        else
        {
            lock.unlock();
            RWMutexType::WirteLock lock2(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (fd_ctx->events & event)
        {
            LJRSERVER_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd
                                          << " event = " << event
                                          << " fd_ctx.events = " << fd_ctx->events;
            LJRSERVER_ASSERT(!(fd_ctx->events & event));
        }

        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epollfd, op, fd, &epevent);
        if (rt)
        {
            LJRSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epollfd << ", "
                                          << op << ", " << fd << ", " << epevent.events << "): "
                                          << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return -1;
        }

        ++m_pendingEventCount;
        fd_ctx->events = (Event)(fd_ctx->events | event);
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        LJRSERVER_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

        event_ctx.scheduler = Scheduler::GetThis();
        if (cb)
        {
            event_ctx.cb.swap(cb);
        }
        else
        {
            event_ctx.fiber = Fiber::GetThis();
            LJRSERVER_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
        }

        return 0;
    }

    // 删除事件
    bool IOManager::delEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }

        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->events & event))
        {
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epollfd, op, fd, &epevent);
        if (rt)
        {
            LJRSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epollfd << ", "
                                          << op << ", " << fd << ", " << epevent.events << "): "
                                          << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        fd_ctx->triggerEvent(event);

        --m_pendingEventCount;
        // FdContext::EventContext &event_ctx = fd_ctx->getContext(event);

        return true;
    }

    // 取消事件
    bool IOManager::cancelEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }

        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->events & event))
        {
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epollfd, op, fd, &epevent);
        if (rt)
        {
            LJRSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epollfd << ", "
                                          << op << ", " << fd << ", " << epevent.events << "): "
                                          << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;

        return true;
    }

    // 取消句柄下所有事件
    bool IOManager::cancelAll(int fd)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }

        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!fd_ctx->events)
        {
            return false;
        }

        // Event new_events = (Event)(fd_ctx->events & ~event);
        // int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        // epevent.events = EPOLLET | new_events;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epollfd, op, fd, &epevent);
        if (rt)
        {
            LJRSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epollfd << ", "
                                          << op << ", " << fd << ", " << epevent.events << "): "
                                          << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        if (fd_ctx->events & READ)
        {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if (fd_ctx->events & WRITE)
        {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        LJRSERVER_ASSERT(fd_ctx->events == 0);
        return true;
    }

    // 获取当前的io管理器
    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    void IOManager::tickle()
    {
        if (!hasIdleThreads())
        {
            return;
        }
        int rt = write(m_tickleFds[1], "T", 1);
        // 成功返回长度
        LJRSERVER_ASSERT(rt == 1);
    }

    bool IOManager::stopping()
    {
        // return m_pendingEventCount == 0 && Scheduler::stopping();
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    bool IOManager::stopping(uint64_t &timeout)
    {
        timeout = getNextTimer();
        return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    void IOManager::idle()
    {
        // 不在栈上创建太大的数组，用指针
        epoll_event *events = new epoll_event[64]();
        // 通过智能指针，释放数组内存
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr) {
            delete[] ptr;
        });

        while (true)
        {
            // if (stopping())
            // {
            //     LJRSERVER_LOG_INFO(g_logger) << "name = " << getName()
            //                                  << " idle stopping exit";
            //     break;
            // }

            uint64_t next_timeout = 0;
            if (stopping(next_timeout))
            {
                // next_timeout = getNextTimer();
                // if (next_timeout == ~0ull)
                // {
                LJRSERVER_LOG_INFO(g_logger) << "name = " << getName()
                                             << " idle stopping exit";
                break;
                // }
            }

            int rt = 0;
            do
            {
                static const int MAX_TIMEOUT = 5000;
                if (next_timeout != ~0ull)
                {
                    next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
                }
                else
                {
                    next_timeout = MAX_TIMEOUT;
                }

                // rt = epoll_wait(m_epollfd, events, 64, MAX_TIMEOUT);
                rt = epoll_wait(m_epollfd, events, 64, (int)next_timeout);

                if (rt < 0 && errno == EINTR)
                {
                }
                else
                {
                    break;
                }

            } while (true);

            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if (!cbs.empty())
            {
                // LJRSERVER_LOG_DEBUG(g_logger) << "on timer cbs.size = " << cbs.size();
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            // 处理事件，rt为事件个数
            for (int i = 0; i < rt; ++i)
            {
                epoll_event &event = events[i];
                if (event.data.fd == m_tickleFds[0])
                {
                    uint8_t dummy;
                    while (read(m_tickleFds[0], &dummy, 1) == 1)
                    {
                    }
                    continue;
                }

                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= EPOLLIN | EPOLLOUT;
                }
                int real_events = NONE;
                if (event.events & EPOLLIN)
                {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT)
                {
                    real_events |= WRITE;
                }

                if ((fd_ctx->events & real_events) == NONE)
                {
                    continue;
                }

                int left_events = (fd_ctx->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                int rt2 = epoll_ctl(m_epollfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    LJRSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epollfd << ", "
                                                  << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "): "
                                                  << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                if (real_events & READ)
                {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE)
                {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }

            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();

            raw_ptr->swapOut();
        }
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }

} // namespace ljrserver
