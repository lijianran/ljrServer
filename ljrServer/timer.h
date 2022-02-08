
#ifndef __LJRSERVER_TIMER_H__
#define __LJRSERVER_TIMER_H__

#include <memory>
#include <vector>
#include <set>
#include "thread.h"

namespace ljrserver {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;

public:
    typedef std::shared_ptr<Timer> ptr;

    bool cancel();

    bool refresh();

    bool reset(uint64_t ms, bool from_now);

private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring,
          TimerManager *manager);

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
    // 所属的timer管理器
    TimerManager *m_manager = nullptr;

private:
    struct Comparator {
        bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
    };
};

class TimerManager {
    friend class Timer;

public:
    typedef RWMutex RWMutexType;

    TimerManager();

    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                        bool recurring = false);

    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                 std::weak_ptr<void> weak_conditon,
                                 bool recurring = false);
    // 下一个定时器的执行时间
    uint64_t getNextTimer();

    // 超时，未执行的任务
    void listExpiredCb(std::vector<std::function<void()>> &cbs);

    bool hasTimer();

protected:
    virtual void onTimerInsertedAtFront() = 0;

    void addTimer(Timer::ptr timer, RWMutexType::WriteLock &lock);

private:
    bool detectClockRollover(uint64_t now_ms);

private:
    RWMutexType m_mutex;

    std::set<Timer::ptr, Timer::Comparator> m_timers;

    bool m_tickled = false;

    uint64_t m_previousTime = 0;
};

}  // namespace ljrserver

#endif  //__LJRSERVER_TIMER_H__