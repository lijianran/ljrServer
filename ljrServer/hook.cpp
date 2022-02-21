
#include "hook.h"

// dlsym
#include <dlfcn.h>

// 日志
#include "log.h"
// 协程
#include "fiber.h"
// IO 管理
#include "iomanager.h"
// 句柄管理
#include "fd_manager.h"
// 配置
#include "config.h"

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

namespace ljrserver {

// 配置 tcp 连接的超时 timeout 5s
static ljrserver::ConfigVar<int>::ptr g_tcp_connect_timeout =
    ljrserver::Config::Lookup("tcp.connect.timeout", 5000,
                              " tcp connect timeout");

// thread_local 线程局部变量 是否开启了 hook [= false]
static thread_local bool t_hook_enable = false;

// 全局静态变量 socket 连接超时参数
static uint64_t s_connect_timeout = -1;

// hook
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

/**
 * @brief 初始化执行函数
 *
 */
void hook_init() {
    static bool is_init = false;
    if (is_init) {
        return;
    }

    // 获取原系统函数
    // sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");

#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
    // XX(sleep) -> sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
    HOOK_FUN(XX);
#undef XX
}

/**
 * @brief 初始化
 *
 */
struct _HookIniter {
    /**
     * @brief 结构体构造函数
     *
     */
    _HookIniter() {
        // 初始化 hook
        hook_init();

        // 设置连接超时参数
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        // 添加变更监听函数
        g_tcp_connect_timeout->addListener([](const int &old_value,
                                              const int &new_value) {
            LJRSERVER_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to  " << new_value;
            // 更新连接超时参数
            s_connect_timeout = new_value;
        });
    }
};

// 在 main 函数之前执行，全局变量初始化在 main 函数之前
static _HookIniter s_hook_initer;

/**
 * @brief 当前线程是否开启了 hook
 *
 * @return true
 * @return false
 */
bool is_hook_enable() {
    // 线程局部变量 thread_local
    return t_hook_enable;
}

/**
 * @brief 设置 hook 状态
 *
 * @param flag 是否开启 hook
 */
void set_hook_enable(bool flag) {
    // 线程局部变量 thread_local
    t_hook_enable = flag;
}

}  // namespace ljrserver

/**
 * @brief 定时器条件
 *
 */
struct timer_info {
    // 是否取消
    int cancelled = 0;
};

/**
 * @brief 模版函数 执行 IO 操作
 *
 * @tparam OriginFun 原系统调用函数
 * @tparam Args 函数参数
 * @param fd 句柄
 * @param fun 原系统调用函数
 * @param hook_fun_name hook 函数名称
 * @param event IO 事件类型
 * @param timeout_so 超时类型
 * @param args 其他参数
 * @return ssize_t
 */
template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args) {
    // 当前线程没有 hook 用系统函数执行返回
    if (!ljrserver::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 获取文件句柄
    ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(fd);
    if (!ctx) {
        // 不存在，用系统函数执行返回
        return fun(fd, std::forward<Args>(args)...);
    }

    // 文件句柄已经被关闭了
    if (ctx->isClosed()) {
        errno = EBADF;
        return -1;
    }

    // 不是 socket 句柄，或者用户已经设置了非阻塞，用系统函数执行返回
    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 获取发送/接收超时
    uint64_t timeout = ctx->getTimeout(timeout_so);
    // 定时器条件 该函数执行完毕后智能指针析构 条件为假
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    // 执行原系统函数，如果 n 非负数，说明已经读到数据，可以直接返回 n
    ssize_t n = fun(fd, std::forward<Args>(args)...);

    // 如果没读到数据，且错误类型为 e intr interrupted 被中断则重试
    while (n == -1 && errno == EINTR) {
        // 如果 n 非负，能读到数据了，也直接返回 n
        n = fun(fd, std::forward<Args>(args)...);
    }

    // 错误类型为 e again 则需要执行异步操作
    if (n == -1 && errno == EAGAIN) {
        LJRSERVER_LOG_DEBUG(g_logger) << "do_io <" << hook_fun_name << ">";

        // 获取此时的 IO 管理器
        ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();
        // 设置定时器
        ljrserver::Timer::ptr timer;
        // 设置条件
        std::weak_ptr<timer_info> winfo(tinfo);

        // 不是永久超时
        if (timeout != (uint64_t)-1) {
            // 设置条件定时器 模拟超时
            timer = iom->addConditionTimer(
                timeout,
                [winfo, fd, iom, event]() {
                    // 返回智能指针 智能指针存在说明 IO 任务仍在后台
                    auto t = winfo.lock();
                    if (!t || t->cancelled) {
                        // 智能指针不存在或者已经取消
                        return;
                    }

                    // e timed out 表明超时取消
                    t->cancelled = ETIMEDOUT;

                    // 取消事件 如果事件存在则触发事件 执行任务
                    iom->cancelEvent(fd, (ljrserver::IOManager::Event)(event));
                },
                winfo);
        }

        // 当前 IO 没消息 注册当前 IO 任务等待后续调度执行
        int rt = iom->addEvent(fd, (ljrserver::IOManager::Event)(event));
        if (rt) {
            // 失败
            LJRSERVER_LOG_ERROR(g_logger)
                << hook_fun_name << " addEvent(" << fd << ", " << event << ")";

            if (timer) {
                // 取消定时器
                timer->cancel();
            }
            return -1;

        } else {
            // 切换至调度协程 过段时间由调度器唤醒再执行 IO 操作
            ljrserver::Fiber::YieldToHold();

            // 唤醒后
            if (timer) {
                // 定时器还存在 则取消定时器
                timer->cancel();
            }

            // 判断是否超时
            if (tinfo->cancelled) {
                // 已经超时 e timed out
                errno = tinfo->cancelled;
                // IO 操作接口返回 -1
                return -1;
            }

            // 没有超时 再次进行 IO 任务查看是否有数据响应
            goto retry;
        }
    }

    return n;
}

/***********************************
 * C 代码
 ***********************************/

extern "C" {

// 定义原系统函数
// sleep_fun sleep_f = nullptr;

// ## 是链接符
#define XX(name) name##_fun name##_f = nullptr;
// XX(sleep) -> sleep_fun sleep_f = nullptr;
HOOK_FUN(XX);
#undef XX

/***********************
 * sleep
 ***********************/

unsigned int sleep(unsigned int seconds) {
    if (!ljrserver::t_hook_enable) {
        // 当前线程没有开启 hook 直接返回原系统调用
        return sleep_f(seconds);
    }

    // 获取当前协程
    ljrserver::Fiber::ptr fiber = ljrserver::Fiber::GetThis();
    // 获取当前 IO 调度管理器
    ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();

    // 添加定时器 sleep 后再调度执行当前协程
    iom->addTimer(seconds * 1000,
                  std::bind((void(ljrserver::Scheduler::*)(
                                ljrserver::Fiber::ptr, int thread)) &
                                ljrserver::IOManager::schedule,
                            iom, fiber, -1));

    // 协程执行函数
    // (void(ljrserver::Scheduler::*)(ljrserver::Fiber::ptr, int thread))
    // &ljrserver::IOManager::schedule

    // iom->addTimer(seconds * 1000, [iom, fiber]() {
    //     iom->schedule(fiber);
    // });

    // 回到调度协程 等待 sleep 结束
    ljrserver::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if (!ljrserver::t_hook_enable) {
        return usleep_f(usec);
    }

    ljrserver::Fiber::ptr fiber = ljrserver::Fiber::GetThis();
    ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();
    iom->addTimer(usec / 1000,
                  std::bind((void(ljrserver::Scheduler::*)(
                                ljrserver::Fiber::ptr, int thread)) &
                                ljrserver::IOManager::schedule,
                            iom, fiber, -1));
    // iom->addTimer(usec / 1000, [iom, fiber]() {
    //     iom->schedule(fiber);
    // });
    ljrserver::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if (!ljrserver::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;

    ljrserver::Fiber::ptr fiber = ljrserver::Fiber::GetThis();
    ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();
    iom->addTimer(timeout_ms,
                  std::bind((void(ljrserver::Scheduler::*)(
                                ljrserver::Fiber::ptr, int thread)) &
                                ljrserver::IOManager::schedule,
                            iom, fiber, -1));
    // iom->addTimer(timeout_ms, [iom, fiber]() {
    //     iom->schedule(fiber);
    // });
    ljrserver::Fiber::YieldToHold();
    return 0;
}

/***********************
 * socket
 ***********************/

/**
 * @brief 创建 socket 句柄
 *
 * @param domain
 * @param type
 * @param protocol
 * @return int
 */
int socket(int domain, int type, int protocol) {
    if (!ljrserver::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }

    // 系统调用创建 socket
    int fd = socket_f(domain, type, protocol);
    if (fd == -1) {
        // 创建失败
        return fd;
    }

    // get 获取句柄对象 有就重复利用 没有则创建
    ljrserver::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

/**
 * @brief socket 连接加上超时取消功能
 *
 * @param fd socket 句柄
 * @param addr 连接地址
 * @param addrlen 地址长度
 * @param timeout_ms 超时
 * @return int
 */
int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,
                         uint64_t timeout_ms) {
    LJRSERVER_LOG_DEBUG(g_logger) << "是否 Hook: " << ljrserver::t_hook_enable;
    if (!ljrserver::t_hook_enable) {
        // 当前线程没有开启 hook 直接返回原系统调用
        return connect_f(fd, addr, addrlen);
    }

    // 获取句柄对象
    ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClosed()) {
        // 句柄不存在或者已经关闭
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket()) {
        // 不是 socket
        return connect_f(fd, addr, addrlen);
    }

    if (ctx->getUserNonblock()) {
        // 如果用户设置了非阻塞 直接返回系统调用
        return connect_f(fd, addr, addrlen);
    }

    // 用户设置了阻塞模式 则需要进行 hook
    int n = connect_f(fd, addr, addrlen);
    if (n == 0) {
        // 连接成功
        return 0;
    } else if (n != -1 || errno != EINPROGRESS) {
        // 连接失败 且不是操作系统中断错误
        return n;
    }

    // 获取当前 IO 管理器
    ljrserver::IOManager *iom = ljrserver::IOManager::GetThis();
    // 定时器
    ljrserver::Timer::ptr timer;
    // 智能指针条件 表示是否完成本次连接操作
    std::shared_ptr<timer_info> tinfo(new timer_info);
    // 定时器条件
    std::weak_ptr<timer_info> winfo(tinfo);

    // 不是永久超时 5s
    if (timeout_ms != (uint64_t)-1) {
        // 添加条件定时器
        timer = iom->addConditionTimer(
            timeout_ms,
            [winfo, fd, iom]() {
                // 取出智能指针 判断是否已经结束连接操作
                auto t = winfo.lock();
                if (!t || t->cancelled) {
                    return;
                }

                // 超时了
                t->cancelled = ETIMEDOUT;

                // 取消连接 如果事件存在则触发事件 执行任务
                iom->cancelEvent(fd, ljrserver::IOManager::WRITE);
            },
            winfo);
    }

    // 注册当前 connect 的操作事件
    int rt = iom->addEvent(fd, ljrserver::IOManager::WRITE);
    if (rt == 0) {
        // 注册成功 移至后台
        ljrserver::Fiber::YieldToHold();

        // IO 管理器 epoll_wait 监测到 fd 有事件 唤醒
        if (timer) {
            // 有定时器则取消
            timer->cancel();
        }

        if (tinfo->cancelled) {
            // 是否已经超时取消
            errno = tinfo->cancelled;
            return -1;
        }

    } else {
        // 注册事件失败
        if (timer) {
            // 取消定时器
            timer->cancel();
        }

        LJRSERVER_LOG_ERROR(g_logger)
            << "connect addEvent(" << fd << ", WRITE) error";
    }

    // 检查是否成功
    int error = 0;
    socklen_t len = sizeof(int);
    // 取出错误
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }

    if (!error) {
        // 无错误 连接成功
        return 0;
    } else {
        // 有错误
        errno = error;
        return -1;
    }
}

/**
 * @brief socket 连接
 *
 * @param sockfd
 * @param addr
 * @param addrlen
 * @return int
 */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // return connect_f(sockfd, addr, addrlen);
    // 连接加上超时取消功能
    return connect_with_timeout(sockfd, addr, addrlen,
                                ljrserver::s_connect_timeout);
}

/**
 * @brief socket accept
 *
 * @param s
 * @param addr
 * @param addrlen
 * @return int
 */
int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", ljrserver::IOManager::READ,
                   SO_RCVTIMEO, addr, addrlen);
    if (fd >= 0) {
        // accept 成功 加入句柄管理器
        ljrserver::FdMgr::GetInstance()->get(fd, true);
    }

    return fd;
}

/***********************
 * socket read
 ***********************/

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", ljrserver::IOManager::READ, SO_RCVTIMEO,
                 buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", ljrserver::IOManager::READ, SO_RCVTIMEO,
                 iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    // return recv_f(sockfd, buf, len, flags);
    return do_io(sockfd, recv_f, "recv", ljrserver::IOManager::READ,
                 SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", ljrserver::IOManager::READ,
                 SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", ljrserver::IOManager::READ,
                 SO_RCVTIMEO, msg, flags);
}

/***********************
 * socket write
 ***********************/

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", ljrserver::IOManager::WRITE, SO_SNDTIMEO,
                 buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", ljrserver::IOManager::WRITE,
                 SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", ljrserver::IOManager::WRITE, SO_SNDTIMEO,
                 msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags,
               const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", ljrserver::IOManager::WRITE,
                 SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", ljrserver::IOManager::WRITE,
                 SO_SNDTIMEO, msg, flags);
}

/***********************
 * socket close
 ***********************/
int close(int fd) {
    if (!ljrserver::t_hook_enable) {
        // 当前线程没有开启 hook 直接返回系统调用
        return close_f(fd);
    }

    // 获取句柄对象
    ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(fd);
    if (ctx) {
        // 获取当前 IO 管理器
        auto iom = ljrserver::IOManager::GetThis();
        if (iom) {
            // 取消所有事件 并执行
            iom->cancelAll(fd);
        }

        // 从句柄管理器中删除句柄对象
        ljrserver::FdMgr::GetInstance()->del(fd);
    }

    // 返回系统调用
    return close_f(fd);
}

/***********************
 * 设置根句柄相关的参数
 ***********************/

int fcntl(int fd, int cmd, ... /* arg */) {
    va_list va;
    va_start(va, cmd);
    switch (cmd) {
        case F_SETFL: {
            // set flags
            int arg = va_arg(va, int);
            va_end(va);

            // 获取句柄对象
            ljrserver::FdCtx::ptr ctx =
                ljrserver::FdMgr::GetInstance()->get(fd);

            // 没有句柄对象 句柄关闭了 不是 socket
            if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
                return fcntl_f(fd, cmd, arg);
            }

            // 用户是否设置了非阻塞
            ctx->setUserNonblock(arg & O_NONBLOCK);

            // 是否设置了系统非阻塞
            if (ctx->getSysNonblock()) {
                // socket 句柄非阻塞
                arg |= O_NONBLOCK;
            } else {
                // 其他句柄阻塞
                arg &= ~O_NONBLOCK;
            }

            // set flags
            return fcntl_f(fd, cmd, arg);
        }
        // break;
        case F_GETFL: {
            // get flags
            va_end(va);
            int arg = fcntl_f(fd, cmd);

            ljrserver::FdCtx::ptr ctx =
                ljrserver::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
                return arg;
            }

            if (ctx->getUserNonblock()) {
                // 用户设置了非阻塞
                return arg | O_NONBLOCK;
            } else {
                // 用户设置了阻塞
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
            // set pipe size 设置管道大小
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        } break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            // get pipe size 获取管道大小
            va_end(va);
            return fcntl_f(fd, cmd);
        } break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK: {
            struct flock *arg = va_arg(va, struct flock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        } break;
        case F_GETOWN_EX:
        case F_SETOWN_EX: {
            struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        } break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    // 解析 ...
    va_list va;
    va_start(va, request);
    void *arg = va_arg(va, void *);
    va_end(va);

    if (FIONBIO == request) {
        bool user_nonblock = !!*(int *)arg;

        // 获取句柄对象
        ljrserver::FdCtx::ptr ctx = ljrserver::FdMgr::GetInstance()->get(d);

        if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
            // 没有句柄对象 句柄关闭了 不是 socket
            return ioctl_f(d, request, arg);
        }

        // 用户是否设置了非阻塞
        ctx->setUserNonblock(user_nonblock);
    }

    return ioctl_f(d, request, arg);
}

/**
 * @brief 获取句柄属性
 *
 * @param sockfd
 * @param level
 * @param optname
 * @param optval
 * @param optlen
 * @return int
 */
int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

/**
 * @brief 设置句柄属性
 *
 * @param sockfd
 * @param level
 * @param optname
 * @param optval
 * @param optlen
 * @return int
 */
int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
    // 当前线程是否开启了 hook
    if (!ljrserver::t_hook_enable) {
        // 当前线程没有开启 hook 返回系统调用
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }

    // socket
    if (level == SOL_SOCKET) {
        // recv send 发送/接收超时
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            // 获取句柄对象
            ljrserver::FdCtx::ptr ctx =
                ljrserver::FdMgr::GetInstance()->get(sockfd);
            if (ctx) {
                // 设置发送/接收超时
                const timeval *v = (const timeval *)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }

    return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}