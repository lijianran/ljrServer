
#ifndef __LJRSERVER_IOMANAGER_H__
#define __LJRSERVER_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace ljrserver
{

    class IOManager : public Scheduler, public TimerManager
    {
    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;

        enum Event
        {
            // 无事件
            NONE = 0x0,
            // 读事件(EPOLLIN)
            READ = 0x1,
            // 写事件(EPOLLOUT)
            WRITE = 0x4,
        };

    private:
        struct FdContext
        {
            typedef Mutex MutexType;

            // 事件上下文
            struct EventContext
            {
                // 执行事件的调度器
                Scheduler *scheduler = nullptr;
                // 事件执行的协程
                Fiber::ptr fiber;
                // 事件的回调函数
                std::function<void()> cb;
            };

            EventContext &getContext(Event event);

            void resetContext(EventContext &ctx);

            void triggerEvent(Event event);

            // 事件句柄
            int fd;
            // 读事件
            EventContext read;
            // 写事件
            EventContext write;
            // 当前事件
            Event events = NONE;
            // 事件的锁
            MutexType mutex;
        };

    public:
        IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");
        ~IOManager();

        // 添加事件，1 success, 2 retry, -1 error
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        // 删除事件
        bool delEvent(int fd, Event event);
        // 取消事件
        bool cancelEvent(int fd, Event event);
        // 取消句柄下所有事件
        bool cancelAll(int fd);
        // 获取当前的io管理器
        static IOManager *GetThis();

    protected:
        void tickle() override;

        bool stopping() override;

        bool stopping(uint64_t &timeout);

        void idle() override;

        void onTimerInsertedAtFront() override;

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

} // namespace ljrserver

#endif // __LJRSERVER_IOMANAGER_H__