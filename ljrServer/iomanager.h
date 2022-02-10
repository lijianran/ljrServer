
#ifndef __LJRSERVER_IOMANAGER_H__
#define __LJRSERVER_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace ljrserver {

/**
 * @brief IO 协程调度管理
 *
 */
class IOManager : public Scheduler, public TimerManager {
public:
    // 智能指针
    typedef std::shared_ptr<IOManager> ptr;

    // 读写锁
    typedef RWMutex RWMutexType;

    // epoll 事件
    enum Event {
        // 无事件
        NONE = 0x0,
        // 读事件 (EPOLLIN)
        READ = 0x1,
        // 写事件 (EPOLLOUT)
        WRITE = 0x4,
    };

private:
    /**
     * @brief 句柄上下文结构体
     *
     */
    struct FdContext {
        // 互斥锁
        typedef Mutex MutexType;

        /**
         * @brief 事件上下文结构体
         *
         */
        struct EventContext {
            // 执行事件的调度器
            Scheduler *scheduler = nullptr;

            // 事件执行的协程
            Fiber::ptr fiber;

            // 事件的回调函数
            std::function<void()> cb;
        };

        /**
         * @brief 获得事件上下文
         *
         * @param event 事件
         * @return EventContext& 事件上下文
         */
        EventContext &getContext(Event event);

        /**
         * @brief 重置事件上下文
         *
         * @param ctx 事件上下文
         */
        void resetContext(EventContext &ctx);

        /**
         * @brief 触发事件
         *
         * @param event epoll 事件
         */
        void triggerEvent(Event event);

        // 事件句柄
        int fd;

        // 读事件上下文
        EventContext read;

        // 写事件上下文
        EventContext write;

        // 当前事件
        Event events = NONE;

        // 锁
        MutexType mutex;
    };

public:
    /**
     * @brief IO 协程调度器的构造函数
     *
     * @param threads 线程数
     * @param use_caller 是否使用 caller 线程 [= true]
     * @param name 调度器名称
     */
    IOManager(size_t threads = 1, bool use_caller = true,
              const std::string &name = "iom");

    /**
     * @brief IO 协程调度器的析构函数
     *
     */
    ~IOManager();

    /**
     * @brief 添加事件
     *
     * 1 success, 2 retry, -1 error
     * @param fd 事件句柄
     * @param event 事件类型
     * @param cb 事件函数 [= nullptr]
     * @return int 0-success
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /**
     * @brief 删除事件 不会触发事件
     *
     * @param fd 事件句柄
     * @param event 事件
     * @return true
     * @return false
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief 取消事件 如果事件存在则触发事件
     *
     * @param fd 事件句柄
     * @param event 事件
     * @return true
     * @return false
     */
    bool cancelEvent(int fd, Event event);

    /**
     * @brief 取消句柄下所有事件 会执行事件
     *
     * @param fd 事件句柄
     * @return true
     * @return false
     */
    bool cancelAll(int fd);

    /**
     * @brief 获取当前的 IO 管理器
     * static
     *
     * @return IOManager*
     */
    static IOManager *GetThis();

protected:
    /**
     * @brief 虚函数的实现 唤醒
     *
     */
    void tickle() override;

    /**
     * @brief 虚函数的实现 是否正在停止
     *
     * @return true
     * @return false
     */
    bool stopping() override;

    /**
     * @brief 虚函数的实现 是否正在停止
     *
     * 定时器实现
     *
     * @param timeout 下一个定时任务执行还要多久
     * @return true
     * @return false
     */
    bool stopping(uint64_t &timeout);

    /**
     * @brief 虚函数的实现 idle_fiber
     *
     */
    void idle() override;

    /**
     * @brief TimerManager 纯虚函数的实现
     *
     * 当有新的定时器插入到定时器的首部 执行该函数 tickle 唤醒线程执行任务
     *
     */
    void onTimerInsertedAtFront() override;

    /**
     * @brief 句柄上下文数组大小调整
     *
     * @param size 数组大小
     */
    void contextResize(size_t size);

private:
    // epoll 句柄
    int m_epollfd = 0;

    // pipe管道
    int m_tickleFds[2];

    // 等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};

    // 锁
    RWMutexType m_mutex;

    // 句柄上下文数组
    std::vector<FdContext *> m_fdContexts;
};

}  // namespace ljrserver

#endif  // __LJRSERVER_IOMANAGER_H__