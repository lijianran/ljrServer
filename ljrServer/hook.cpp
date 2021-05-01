
#include "hook.h"
#include <dlfcn.h>

#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "config.h"

static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

namespace ljrserver
{

    static ljrserver::ConfigVar<int>::ptr g_tcp_connect_timeout =
        ljrserver::Config::Lookup("tcp.connect.timeout", 5000, " tcp connect timeout");

    static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

    void hook_init()
    {
        static bool is_init = false;
        if (is_init)
        {
            return;
        }

#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
#undef XX
    }

    static uint64_t s_connect_timeout = -1;

    struct _HookIniter
    {
        _HookIniter()
        {
            hook_init();

            s_connect_timeout = g_tcp_connect_timeout->getValue();

            g_tcp_connect_timeout->addListener([](const int &old_value, const int &new_value) {
                LJRSERVER_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                             << old_value << " to  " << new_value;
                s_connect_timeout = new_value;
            });
        }
    };
    // 在main函数之前执行，全局变量初始化在main函数之前
    static _HookIniter s_hook_initer;

    bool is_hook_enable()
    {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag)
    {
        t_hook_enable = flag;
    }

} // namespace ljrserver

struct timer_info
{
    int cancelled = 0;
};

template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args)
{
    // 没有hook，用系统函数执行返回
    if (!ljrserver::t_hook_enable)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 文件句柄不存在，用系统函数执行返回
    ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(fd);
    if (!ctx)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 文件句柄已经被关闭了
    if (ctx->isClosed())
    {
        errno = EBADF;
        return -1;
    }

    // 不是socket句柄，或者用户已经设置了非阻塞，用系统函数执行返回
    if (!ctx->isSocket() || ctx->getUserNonblock())
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t timeout = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    // 直接原函数执行，如果n非负数，说明已经读到数据，可以直接返回n
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    // 如果没读到数据，且错误类型为 interrupted ，重试
    while (n == -1 && errno == EINTR)
    {
        // 如果n非负，能读到数据了，也直接返回n
        n = fun(fd, std::forward<Args>(args)...);
    }
    // 错误类型为 EAGAIN 则需要执行异步操作
    if (n == -1 && errno == EAGAIN)
    {
        LJRSERVER_LOG_DEBUG(g_logger) << "do_io <" << hook_fun_name << ">";

        // 拿到此时的IO管理器
        ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();
        // 设置定时器
        ljrserver::Timer::ptr timer;
        // 设置条件
        std::weak_ptr<timer_info> winfo(tinfo);

        if (timeout != (uint64_t)-1)
        {
            timer = iom->addConditionTimer(
                timeout, [winfo, fd, iom, event]() {
                    auto t = winfo.lock();
                    if (!t || t->cancelled)
                    {
                        return;
                    }

                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (ljrserver::IOManager::Event)(event));
                },
                winfo);
        }

        int rt = iom->addEvent(fd, (ljrserver::IOManager::Event)(event));
        if (rt)
        {
            LJRSERVER_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                                          << fd << ", " << event << ")";

            if (timer)
            {
                timer->cancel();
            }
            return -1;
        }
        else
        {
            ljrserver::Fiber::YieldToHold();
            if (timer)
            {
                timer->cancel();
            }
            if (tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }

            goto retry;
        }
    }

    return n;
}

extern "C"
{
// ##是链接符
#define XX(name) name##_fun name##_f = nullptr;
    HOOK_FUN(XX);
#undef XX

    unsigned int sleep(unsigned int seconds)
    {
        if (!ljrserver::t_hook_enable)
        {
            return sleep_f(seconds);
        }

        ljrserver::Fiber::ptr fiber = ljrserver::Fiber::GetThis();
        ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();
        iom->addTimer(seconds * 1000, std::bind((void (ljrserver::Scheduler::*)(ljrserver::Fiber::ptr, int thread)) & ljrserver::IOManager::schedule, iom, fiber, -1));
        // iom->addTimer(seconds * 1000, [iom, fiber]() {
        //     iom->schedule(fiber);
        // });
        ljrserver::Fiber::YieldToHold();
        return 0;
    }

    int usleep(useconds_t usec)
    {
        if (!ljrserver::t_hook_enable)
        {
            return usleep_f(usec);
        }

        ljrserver::Fiber::ptr fiber = ljrserver::Fiber::GetThis();
        ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();
        iom->addTimer(usec / 1000, std::bind((void (ljrserver::Scheduler::*)(ljrserver::Fiber::ptr, int thread)) & ljrserver::IOManager::schedule, iom, fiber, -1));
        // iom->addTimer(usec / 1000, [iom, fiber]() {
        //     iom->schedule(fiber);
        // });
        ljrserver::Fiber::YieldToHold();
        return 0;
    }

    int nanosleep(const struct timespec *req, struct timespec *rem)
    {
        if (!ljrserver::t_hook_enable)
        {
            return nanosleep_f(req, rem);
        }

        int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;

        ljrserver::Fiber::ptr fiber = ljrserver::Fiber::GetThis();
        ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();
        iom->addTimer(timeout_ms, std::bind((void (ljrserver::Scheduler::*)(ljrserver::Fiber::ptr, int thread)) & ljrserver::IOManager::schedule, iom, fiber, -1));
        // iom->addTimer(timeout_ms, [iom, fiber]() {
        //     iom->schedule(fiber);
        // });
        ljrserver::Fiber::YieldToHold();
        return 0;
    }

    int socket(int domain, int type, int protocol)
    {
        if (!ljrserver::t_hook_enable)
        {
            return socket_f(domain, type, protocol);
        }

        int fd = socket_f(domain, type, protocol);
        if (fd == -1)
        {
            return fd;
        }

        ljrserver::FdMgr::GetInstance()->get(fd, true);
        return fd;
    }

    int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms)
    {
        LJRSERVER_LOG_DEBUG(g_logger) << ljrserver::t_hook_enable;
        if (!ljrserver::t_hook_enable)
        {
            return connect_f(fd, addr, addrlen);
        }

        ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClosed())
        {
            errno = EBADF;
            return -1;
        }

        if (!ctx->isSocket())
        {
            return connect_f(fd, addr, addrlen);
        }

        if (ctx->getUserNonblock())
        {
            return connect_f(fd, addr, addrlen);
        }

        int n = connect_f(fd, addr, addrlen);
        if (n == 0)
        {
            return 0;
        }
        else if (n != -1 || errno != EINPROGRESS)
        {
            return n;
        }

        ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();
        ljrserver::Timer::ptr timer;
        std::shared_ptr<timer_info> tinfo(new timer_info);
        std::weak_ptr<timer_info> winfo(tinfo);

        if (timeout_ms != (uint64_t)-1)
        {
            timer = iom->addConditionTimer(
                timeout_ms, [winfo, fd, iom]() {
                    auto t = winfo.lock();
                    if (!t || t->cancelled)
                    {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, ljrserver::IOManager::WRITE);
                },
                winfo);
        }

        int rt = iom->addEvent(fd, ljrserver::IOManager::WRITE);
        if (rt == 0)
        {
            ljrserver::Fiber::YieldToHold();
            if (timer)
            {
                timer->cancel();
            }
            if (tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
        }
        else
        {
            if (timer)
            {
                timer->cancel();
            }
            LJRSERVER_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
        }

        int error = 0;
        socklen_t len = sizeof(int);
        if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
        {
            return -1;
        }

        if (!error)
        {
            return 0;
        }
        else
        {
            errno = error;
            return -1;
        }
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    {
        // return connect_f(sockfd, addr, addrlen);
        return connect_with_timeout(sockfd, addr, addrlen, ljrserver::s_connect_timeout);
    }

    int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
    {
        int fd = do_io(s, accept_f, "accept", ljrserver::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0)
        {
            ljrserver::FdMgr::GetInstance()->get(fd, true);
        }

        return fd;
    }

    // socket read
    ssize_t read(int fd, void *buf, size_t count)
    {
        return do_io(fd, read_f, "read", ljrserver::IOManager::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, readv_f, "readv", ljrserver::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        // return recv_f(sockfd, buf, len, flags);
        return do_io(sockfd, recv_f, "recv", ljrserver::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
    {
        return do_io(sockfd, recvfrom_f, "recvfrom", ljrserver::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
    {
        return do_io(sockfd, recvmsg_f, "recvmsg", ljrserver::IOManager::READ, SO_RCVTIMEO, msg, flags);
    }

    // socket write
    ssize_t write(int fd, const void *buf, size_t count)
    {
        return do_io(fd, write_f, "write", ljrserver::IOManager::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, writev_f, "writev", ljrserver::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int s, const void *msg, size_t len, int flags)
    {
        return do_io(s, send_f, "send", ljrserver::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
    }

    ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
    {
        return do_io(s, sendto_f, "sendto", ljrserver::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
    }

    ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
    {
        return do_io(s, sendmsg_f, "sendmsg", ljrserver::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
    }

    // socket close
    int close(int fd)
    {
        if (!ljrserver::t_hook_enable)
        {
            return close_f(fd);
        }

        ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(fd);
        if (ctx)
        {
            auto iom = ljrserver::IOManager::GetThis();
            if (iom)
            {
                iom->cancelAll(fd);
            }
            ljrserver::FdMgr::GetInstance()->del(fd);
        }

        return close_f(fd);
    }

    // 设置根句柄相关的参数
    int fcntl(int fd, int cmd, ... /* arg */)
    {
        va_list va;
        va_start(va, cmd);
        switch (cmd)
        {
        case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClosed() || !ctx->isSocket())
            {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);

            if (ctx->getSysNonblock())
            {
                arg |= O_NONBLOCK;
            }
            else
            {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
        // break;
        case F_GETFL:
        {
            va_end(va);
            int arg = fcntl_f(fd, cmd);
            ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClosed() || !ctx->isSocket())
            {
                return arg;
            }

            if (ctx->getUserNonblock())
            {
                return arg | O_NONBLOCK;
            }
            else
            {
                return arg & ~O_NONBLOCK;
            }
        }
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return fcntl_f(fd, cmd);
        }
        break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock *arg = va_arg(va, struct flock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
        {
            struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
        }
    }

    int ioctl(int d, unsigned long int request, ...)
    {
        va_list va;
        va_start(va, request);
        void *arg = va_arg(va, void *);
        va_end(va);

        if (FIONBIO == request)
        {
            bool user_nonblock = !!*(int *)arg;
            ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(d);
            if (!ctx || ctx->isClosed() || !ctx->isSocket())
            {
                return ioctl_f(d, request, arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(d, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
    {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
    {
        if (!ljrserver::t_hook_enable)
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        if (level == SOL_SOCKET)
        {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
            {
                ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(sockfd);
                if (ctx)
                {
                    const timeval *v = (const timeval *)optval;
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}