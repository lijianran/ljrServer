
#ifndef __LJRSERVER_THREAD_H__
#define __LJRSERVER_THREAD_H__

// 线程
// #include <thread>
#include <pthread.h>
// 函数
#include <functional>
// 智能指针
#include <memory>
// #include <stdint.h>
// 信号量
#include <semaphore.h>
// 原子量
#include <atomic>
// 禁用复制
#include "noncopyable.h"

namespace ljrserver {

/****************************
 * semaphore 信号量
 ****************************/

/**
 * @brief Class 信号量
 *
 */
class Semaphore : Noncopyable {
public:
    /**
     * @brief 信号量构造函数
     *
     * @param count
     */
    Semaphore(uint32_t count = 0);

    /**
     * @brief 信号量析构函数
     *
     */
    ~Semaphore();

    /**
     * @brief P 操作 -1
     *
     */
    void wait();

    /**
     * @brief V 操作 +1
     *
     */
    void notify();

    // private:
    //     Semaphore(const Semaphore &) = delete;
    //     Semaphore(const Semaphore &&) = delete;
    //     Semaphore &operator=(const Semaphore &) = delete;

private:
    // 信号量
    sem_t m_semaphore;
};

/****************************
 * RAII
 * 面向概念编程 (Concept-Oriented Programming, COP)
 ****************************/

/**
 * @brief 使用锁的模版类
 *
 * @tparam T
 */
template <class T>
struct ScopedLockImpl {
public:
    /**
     * @brief Construct a new Scoped Lock Impl object
     *
     * 构造函数上锁
     *
     * @param mutex
     */
    ScopedLockImpl(T &mutex) : m_mutex(mutex) {
        // lock 概念
        m_mutex.lock();
        m_locked = true;
    }

    /**
     * @brief Destroy the Scoped Lock Impl object
     *
     * 析构函数解锁
     */
    ~ScopedLockImpl() { unlock(); }

    /**
     * @brief 概念上锁
     *
     */
    void lock() {
        if (!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    /**
     * @brief 概念解锁
     *
     */
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    // 锁实例
    T &m_mutex;

    // 是否上锁
    bool m_locked;
};

/**
 * @brief 使用 read 读锁的模版类
 *
 * @tparam T
 */
template <class T>
struct ReadScopedLockImpl {
public:
    /**
     * @brief Construct a new Read Scoped Lock Impl object
     *
     * 构造函数上读锁
     *
     * @param mutex
     */
    ReadScopedLockImpl(T &mutex) : m_mutex(mutex) {
        // rdlock 概念
        m_mutex.rdlock();
        m_locked = true;
    }

    /**
     * @brief Destroy the Read Scoped Lock Impl object
     *
     * 析构函数解读锁
     */
    ~ReadScopedLockImpl() { unlock(); }

    /**
     * @brief 概念上读锁
     *
     */
    void lock() {
        if (!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    /**
     * @brief 概念解读锁
     *
     */
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    // 读锁实例
    T &m_mutex;

    // 是否上锁
    bool m_locked;
};

/**
 * @brief 使用 write 写锁的模版类
 *
 * @tparam T
 */
template <class T>
struct WriteScopedLockImpl {
public:
    /**
     * @brief Construct a new Write Scoped Lock Impl object
     *
     * 构造函数上写锁
     *
     * @param mutex
     */
    WriteScopedLockImpl(T &mutex) : m_mutex(mutex) {
        // wrlock 概念
        m_mutex.wrlock();
        m_locked = true;
    }

    /**
     * @brief Destroy the Write Scoped Lock Impl object
     *
     * 析构函数解锁
     */
    ~WriteScopedLockImpl() { unlock(); }

    /**
     * @brief 概念上写锁
     *
     */
    void lock() {
        if (!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    /**
     * @brief 概念解写锁
     *
     */
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    // 写锁实例
    T &m_mutex;

    // 是否上锁
    bool m_locked;
};

/****************************
 * p_thread 锁的封装
 ****************************/

/**
 * @brief 互斥锁
 *
 */
class Mutex : Noncopyable {
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex() { pthread_mutex_init(&m_mutex, nullptr); }

    ~Mutex() { pthread_mutex_destroy(&m_mutex); }

    void lock() { pthread_mutex_lock(&m_mutex); }

    void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
    pthread_mutex_t m_mutex;
};

class NullMutex : Noncopyable {
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};

/**
 * @brief 读写锁
 *
 */
class RWMutex : Noncopyable {
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }

    ~RWMutex() { pthread_rwlock_destroy(&m_lock); }

    void rdlock() { pthread_rwlock_rdlock(&m_lock); }

    void wrlock() { pthread_rwlock_wrlock(&m_lock); }

    void unlock() { pthread_rwlock_unlock(&m_lock); }

private:
    pthread_rwlock_t m_lock;
};

class NullRWMutex : Noncopyable {
public:
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    NullRWMutex() {}
    ~NullRWMutex() {}
    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};

/**
 * @brief 自旋锁
 *
 * 性能提升3倍，20～30 M/s
 */
class Spinlock : Noncopyable {
public:
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock() { pthread_spin_init(&m_mutex, 0); }

    ~Spinlock() { pthread_spin_destroy(&m_mutex); }

    void lock() { pthread_spin_lock(&m_mutex); }

    void unlock() { pthread_spin_unlock(&m_mutex); }

private:
    pthread_spinlock_t m_mutex;
};

/**
 * @brief Class 原子锁，和自旋锁性能差不多
 *
 * Compare And Swap (CAS)
 */
class CASLock : Noncopyable {
public:
    typedef ScopedLockImpl<CASLock> Lock;

    CASLock() { m_mutex.clear(); }

    ~CASLock() {}

    void lock() {
        while (std::atomic_flag_test_and_set_explicit(
            &m_mutex, std::memory_order_acquire)) {
        }
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }

private:
    // volatile 每次访问时都必须从内存中取出值
    volatile std::atomic_flag m_mutex;
};

/****************************
 * p_thread 线程的封装
 ****************************/

/**
 * @brief 线程
 *
 * 封装 p_thread
 */
class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;

    /**
     * @brief 线程构造函数
     *
     * @param cb 执行函数
     * @param name 线程名称
     */
    Thread(std::function<void()> cb, const std::string &name);

    /**
     * @brief 线程析构函数
     *
     */
    ~Thread();

    /**
     * @brief 获取线程 id
     *
     * @return pid_t
     */
    pid_t getId() const { return m_id; }

    /**
     * @brief 获取线程名称
     *
     * @return const std::string&
     */
    const std::string &getName() const { return m_name; }

    /**
     * @brief 线程 join
     *
     */
    void join();

    /**
     * @brief 获取当前线程的实例对象
     *
     * static 一个当前线程只有一个实例对象
     *
     * @return Thread*
     */
    static Thread *GetThis();

    /**
     * @brief 获取当前线程的名称
     *
     * static 一个当前线程只有一个名称
     *
     * @return const std::string&
     */
    static const std::string &GetName();

    /**
     * @brief 设置当前线程的名称
     *
     * static
     *
     * @param name 线程名称
     */
    static void SetName(const std::string &name);

private:
    // 线程禁用复制移动赋值
    Thread(const Thread &) = delete;
    Thread(const Thread &&) = delete;
    Thread &operator=(const Thread &) = delete;

    /**
     * @brief 线程启动函数
     *
     * static 一个当前线程只有一个执行函数
     *
     * @param arg 函数参数
     * @return void*
     */
    static void *run(void *arg);

private:
    // 线程 id
    pid_t m_id = -1;
    // 线程具柄
    pthread_t m_thread = 0;
    // 线程函数
    std::function<void()> m_cb;
    // 线程名称
    std::string m_name;
    // 信号量
    Semaphore m_semaphore;
};

}  // namespace ljrserver

#endif