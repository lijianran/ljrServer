
#ifndef __LJRSERVER_TIMER_H__
#define __LJRSERVER_TIMER_H__

#include <memory>
#include <vector>
#include <set>
#include "thread.h"

namespace ljrserver {

// 管理器
class TimerManager;

/**
 * @brief Class 定时器
 *
 */
class Timer : public std::enable_shared_from_this<Timer> {
    // 友元类
    friend class TimerManager;

public:
    // 智能指针
    typedef std::shared_ptr<Timer> ptr;

    /**
     * @brief 取消定时器
     *
     * @return true
     * @return false
     */
    bool cancel();

    /**
     * @brief 刷新定时器
     *
     * @return true
     * @return false
     */
    bool refresh();

    /**
     * @brief 重设定时器
     *
     * @param ms 执行周期
     * @param from_now 是否从现在开始记时
     * @return true
     * @return false
     */
    bool reset(uint64_t ms, bool from_now);

private:
    /**
     * @brief 重载定时器构造函数 private
     *
     * @param ms
     * @param cb
     * @param recurring
     * @param manager
     */
    Timer(uint64_t ms, std::function<void()> cb, bool recurring,
          TimerManager *manager);

    /**
     * @brief 重载定时器构造函数 private
     *
     * 用于实例化当前时间的定时器对象
     * Timer::ptr now_timer(new Timer(now_ms));
     *
     * @param next
     */
    Timer(uint64_t next);

private:
    // 是否循环定时器
    bool m_recurring = false;

    // 执行周期
    uint64_t m_ms = 0;

    // 精确的执行时间
    uint64_t m_next = 0;

    // 回调函数，定时器需要执行的任务
    std::function<void()> m_cb;

    // 所属的 timer 管理器
    TimerManager *m_manager = nullptr;

private:
    /**
     * @brief 用于管理器 set 集合排序
     *
     */
    struct Comparator {
        /**
         * @brief 重载 () 用于管理器 set 集合排序
         *
         */
        bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
    };
};

/**
 * @brief Class 管理器
 *
 */
class TimerManager {
    // 友元类
    friend class Timer;

public:
    // 读写锁
    typedef RWMutex RWMutexType;

    /**
     * @brief 管理器构造函数
     *
     */
    TimerManager();

    /**
     * @brief 管理器析构函数 虚函数
     *
     */
    virtual ~TimerManager();

    /**
     * @brief 添加定时器
     *
     * @param ms 执行周期
     * @param cb 任务函数
     * @param recurring 是否循环执行 [= false]
     * @return Timer::ptr
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                        bool recurring = false);

    /**
     * @brief 添加条件定时器
     *
     * @param ms 执行周期
     * @param cb 任务函数
     * @param weak_conditon 执行条件 weak_ptr
     * @param recurring 是否循环执行 [= false]
     * @return Timer::ptr
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                 std::weak_ptr<void> weak_conditon,
                                 bool recurring = false);

    /**
     * @brief 下一个定时器任务还要多久执行
     *
     * @return uint64_t 时间
     */
    uint64_t getNextTimer();

    /**
     * @brief 获取需要执行的定时器的回调函数列表
     *
     * 包括超时，未执行的任务
     *
     * @param cbs 回调函数数组
     */
    void listExpiredCb(std::vector<std::function<void()>> &cbs);

    /**
     * @brief 是否有定时器
     *
     * @return true 还有定时器
     * @return false 暂无定时器
     */
    bool hasTimer();

protected:
    /**
     * @brief 纯虚函数 管理器类是抽象类
     *
     * 当有新的定时器插入到定时器的首部 执行该函数
     *
     */
    virtual void onTimerInsertedAtFront() = 0;

    /**
     * @brief 添加定时器 protected
     *
     * @param timer 定时器对象
     * @param lock 写锁
     */
    void addTimer(Timer::ptr timer, RWMutexType::WriteLock &lock);

private:
    /**
     * @brief 检测服务器时间是否被调后了
     *
     * @param now_ms 当前时间
     * @return true
     * @return false
     */
    bool detectClockRollover(uint64_t now_ms);

private:
    // 读写锁
    RWMutexType m_mutex;

    // 定时器 set 集合
    std::set<Timer::ptr, Timer::Comparator> m_timers;

    // tickle
    bool m_tickled = false;

    // 上次执行时间
    uint64_t m_previousTime = 0;
};

}  // namespace ljrserver

#endif  //__LJRSERVER_TIMER_H__