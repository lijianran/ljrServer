
#include "timer.h"
#include "util.h"

namespace ljrserver {

/**
 * @brief 重载 () 用于管理器 set 集合排序
 *
 */
bool Timer::Comparator::operator()(const Timer::ptr &lhs,
                                   const Timer::ptr &rhs) const {
    // 比较智能指针
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }

    // 比较下一次执行时间
    if (lhs->m_next < rhs->m_next) {
        return true;
    }
    if (rhs->m_next < lhs->m_next) {
        return false;
    }

    // 比较裸指针
    return lhs.get() < rhs.get();
}

/**
 * @brief 重载定时器构造函数 private
 *
 * @param next
 */
Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring,
             TimerManager *manager)
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager) {
    // 设置下一次执行的时间
    m_next = ljrserver::GetCurrentMS() + m_ms;
}

/**
 * @brief 重载定时器构造函数 private
 *
 * 用于实例化当前时间的定时器对象
 * Timer::ptr now_timer(new Timer(now_ms));
 *
 * @param next
 */
Timer::Timer(uint64_t next) : m_next(next) {
    // 初始化列表 m_next 为了能够比较大小
}

/**
 * @brief 取消定时器
 *
 * @return true
 * @return false
 */
bool Timer::cancel() {
    // 友元类 直接操作管理器的锁
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);

    // 是否有定时函数任务
    if (m_cb) {
        // 删除函数任务
        m_cb = nullptr;

        // 友元类 直接在管理器的定时器 set 集合中找到当前的定时器
        auto it = m_manager->m_timers.find(shared_from_this());

        // 友元类 从管理器的 set 集合中删除 erase
        m_manager->m_timers.erase(it);

        // 取消成功
        return true;
    }

    // 没有函数任务 取消失败
    return false;
}

/**
 * @brief 刷新定时器
 *
 * @return true
 * @return false
 */
bool Timer::refresh() {
    // 友元类 直接操作管理器的锁
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);

    if (!m_cb) {
        // 没有定时任务
        return false;
    }

    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        // 管理器中没有找到定时器
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

/**
 * @brief 重设定时器
 *
 * @param ms 执行周期
 * @param from_now 是否从现在开始记时
 * @return true
 * @return false
 */
bool Timer::reset(uint64_t ms, bool from_now) {
    if (ms == m_ms && !from_now) {
        // 周期一样且不从当前开始
        return true;
    }

    // 友元类 直接操作管理器的锁
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        // 没有定时任务
        return false;
    }

    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        // 管理器中没有找到定时器
        return false;
    }

    // 先删除
    m_manager->m_timers.erase(it);

    // 新定时器的开始时间
    uint64_t start = 0;
    if (from_now) {
        // 从现在开始
        start = ljrserver::GetCurrentMS();
    } else {
        // 从上次执行时间开始计算
        start = m_next - m_ms;
    }

    // 设置执行周期
    m_ms = ms;
    // 下次执行时间
    m_next = start + m_ms;

    // 由管理器添加定时器
    m_manager->addTimer(shared_from_this(), lock);

    return true;
}

/**
 * @brief 管理器构造函数
 *
 */
TimerManager::TimerManager() {
    // 初始化执行时间
    m_previousTime = ljrserver::GetCurrentMS();
}

/**
 * @brief 管理器析构函数 虚函数
 *
 */
TimerManager::~TimerManager() {}

/**
 * @brief 添加定时器
 *
 * @param ms 执行周期
 * @param cb 任务函数
 * @param recurring 是否循环执行 [= false]
 * @return Timer::ptr
 */
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                                  bool recurring) {
    // 实例化一个定时器对象
    Timer::ptr timer(new Timer(ms, cb, recurring, this));

    // 上写锁
    RWMutexType::WriteLock lock(m_mutex);

    // 添加定时器 protected
    addTimer(timer, lock);

    return timer;
}

// weak_ptr 不会让引用计数加1，但可以知道指向的内容是否被释放
static void OnTimer(std::weak_ptr<void> weak_condition,
                    std::function<void()> cb) {
    // 返回智能指针
    std::shared_ptr<void> tmp = weak_condition.lock();

    // 智能指针没被释放，则执行回调函数
    if (tmp) {
        cb();
    }
}

/**
 * @brief 添加条件定时器
 *
 * @param ms 执行周期
 * @param cb 任务函数
 * @param weak_conditon 执行条件 weak_ptr
 * @param recurring 是否循环执行 [= false]
 * @return Timer::ptr
 */
Timer::ptr TimerManager::addConditionTimer(uint64_t ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> weak_conditon,
                                           bool recurring) {
    // 添加定时器
    return addTimer(ms, std::bind(&OnTimer, weak_conditon, cb), recurring);
}

/**
 * @brief 下一个定时器任务还要多久执行
 *
 * @return uint64_t 时间
 */
uint64_t TimerManager::getNextTimer() {
    // 上读锁
    RWMutexType::ReadLock lock(m_mutex);

    // 不需要 tickle
    m_tickled = false;

    // 没有定时器
    if (m_timers.empty()) {
        return ~0ull;
    }

    // 最近的定时器
    const Timer::ptr &next = *m_timers.begin();
    // 当前时间
    uint64_t now_ms = ljrserver::GetCurrentMS();

    // 最近一个任务的执行时间已经过了
    if (now_ms >= next->m_next) {
        return 0;
    } else {
        // 最近的任务还要多久执行
        return next->m_next - now_ms;
    }
}

/**
 * @brief 获取需要执行的定时器的回调函数列表
 *
 * 包括超时，未执行的任务
 *
 * @param cbs 回调函数数组
 */
void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
    // 获取当前时间
    uint64_t now_ms = ljrserver::GetCurrentMS();
    // 过期定时器 即要执行的定时任务
    std::vector<Timer::ptr> expired;

    {
        // 上读锁
        RWMutexType::ReadLock lock(m_mutex);
        // 没有定时器则直接返回
        if (m_timers.empty()) {
            return;
        }
    }

    // 上读锁
    RWMutexType::WriteLock lock(m_mutex);

    // 检测服务器时间是否被调后了
    bool rollover = detectClockRollover(now_ms);
    if (!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        // 系统时间没有问题且下次任务时间还没到
        return;
    }

    // 当前时间定时器
    Timer::ptr now_timer(new Timer(now_ms));
    // 迭代器 lower_bound 小于或等于
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);

    while (it != m_timers.end() && (*it)->m_next == now_ms) {
        // 等于的定时器跳过 还没超时
        ++it;
    }

    // 加入任务列表
    expired.insert(expired.begin(), m_timers.begin(), it);
    // 删除定时器
    m_timers.erase(m_timers.begin(), it);
    // 重设 vector 的大小
    cbs.reserve(expired.size());

    // 遍历过期的定时器
    for (auto &timer : expired) {
        // 获取定时任务 加入列表
        cbs.push_back(timer->m_cb);

        // 是否循环
        if (timer->m_recurring) {
            // 下次执行时间
            timer->m_next = now_ms + timer->m_ms;
            // 加入定时器
            m_timers.insert(timer);
        } else {
            // 不循环
            timer->m_cb = nullptr;
        }
    }
}

/**
 * @brief 是否有定时器
 *
 * @return true 还有定时器
 * @return false 暂无定时器
 */
bool TimerManager::hasTimer() {
    // 上读锁
    RWMutexType::ReadLock lock(m_mutex);

    // 定时器 set 集合是否不为空
    return !m_timers.empty();
}

/**
 * @brief 添加定时器 protected
 *
 * @param timer 定时器对象
 * @param lock 写锁
 */
void TimerManager::addTimer(Timer::ptr timer, RWMutexType::WriteLock &lock) {
    // insert 到定时器 set 集合中
    auto it = m_timers.insert(timer).first;

    // 是否在第一个 说明之前没有定时器
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if (at_front) {
        // 需要 tickle
        m_tickled = true;
    }

    // 解开写锁
    lock.unlock();

    if (at_front) {
        // 之前没有定时器 现在有定时任务了 需要 tickle 唤醒
        onTimerInsertedAtFront();
    }
}

/**
 * @brief 检测服务器时间是否被调后了
 *
 * @param now_ms 当前时间
 * @return true
 * @return false
 */
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if (now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    // 设置时间
    m_previousTime = now_ms;

    return rollover;
}

}  // namespace ljrserver
