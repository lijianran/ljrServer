
#include "timer.h"
#include "util.h"

namespace ljrserver
{

    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const
    {
        if (!lhs && !rhs)
        {
            return false;
        }
        if (!lhs)
        {
            return true;
        }
        if (!rhs)
        {
            return false;
        }
        if (lhs->m_next < rhs->m_next)
        {
            return true;
        }
        if (rhs->m_next < lhs->m_next)
        {
            return false;
        }
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
        : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager)
    {
        m_next = ljrserver::GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next) : m_next(next)
    {
    }

    bool Timer::cancel()
    {
        TimerManager::RWMutexType::WirteLock lock(m_manager->m_mutex);
        if (m_cb)
        {
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    bool Timer::refresh()
    {
        TimerManager::RWMutexType::WirteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())
        {
            return false;
        }
        // 先删除
        m_manager->m_timers.erase(it);
        // 重置时间
        m_next = ljrserver::GetCurrentMS() + m_ms;
        // 再加入
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    bool Timer::reset(uint64_t ms, bool from_now)
    {
        if (ms == m_ms && !from_now)
        {
            return true;
        }

        TimerManager::RWMutexType::WirteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }

        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())
        {
            return false;
        }
        // 先删除
        m_manager->m_timers.erase(it);

        uint64_t start = 0;
        if (from_now)
        {
            start = ljrserver::GetCurrentMS();
        }
        else
        {
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;

        m_manager->addTimer(shared_from_this(), lock);

        return true;
    }

    TimerManager::TimerManager()
    {
        m_previousTime = ljrserver::GetCurrentMS();
    }

    TimerManager::~TimerManager()
    {
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
    {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        RWMutexType::WirteLock lock(m_mutex);

        addTimer(timer, lock);

        return timer;
    }

    // weak_ptr 不会让引用计数加1，但可以知道指向的内容是否被释放
    static void OnTimer(std::weak_ptr<void> weak_condition, std::function<void()> cb)
    {
        // 返回智能指针
        std::shared_ptr<void> tmp = weak_condition.lock();
        // 智能指针没被释放，则执行回调函数
        if (tmp)
        {
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                                               std::weak_ptr<void> weak_conditon, bool recurring)
    {
        return addTimer(ms, std::bind(&OnTimer, weak_conditon, cb), recurring);
    }

    // 下一个定时器的执行时间
    uint64_t TimerManager::getNextTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);

        m_tickled = false;

        if (m_timers.empty())
        {
            return ~0ull;
        }

        const Timer::ptr &next = *m_timers.begin();
        uint64_t now_ms = ljrserver::GetCurrentMS();
        if (now_ms >= next->m_next)
        {
            return 0;
        }
        else
        {
            // 还要多久执行
            return next->m_next - now_ms;
        }
    }

    // 超时，未执行的任务
    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now_ms = ljrserver::GetCurrentMS();
        std::vector<Timer::ptr> expired;

        {
            RWMutexType::ReadLock lock(m_mutex);
            if (m_timers.empty())
            {
                return;
            }
        }

        RWMutexType::WirteLock lock(m_mutex);

        bool rollover = detectClockRollover(now_ms);
        if (!rollover && ((*m_timers.begin())->m_next > now_ms))
        {
            return;
        }

        Timer::ptr now_timer(new Timer(now_ms));
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);

        while (it != m_timers.end() && (*it)->m_next == now_ms)
        {
            ++it;
        }
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        cbs.reserve(expired.size());

        for (auto &timer : expired)
        {
            cbs.push_back(timer->m_cb);
            if (timer->m_recurring)
            {
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }
            else
            {
                timer->m_cb = nullptr;
            }
        }
    }

    bool TimerManager::hasTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

    void TimerManager::addTimer(Timer::ptr timer, RWMutexType::WirteLock &lock)
    {
        auto it = m_timers.insert(timer).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if (at_front)
        {
            m_tickled = true;
        }

        lock.unlock();

        if (at_front)
        {
            onTimerInsertedAtFront();
        }
    }

    bool TimerManager::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        if (now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000))
        {
            rollover = true;
        }
        m_previousTime = now_ms;
        return rollover;
    }

} // namespace ljrserver
