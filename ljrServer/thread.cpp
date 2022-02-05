
#include "thread.h"
#include "log.h"
#include "util.h"

namespace ljrserver {

// 线程变量
static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/****************************
 * semaphore 信号量
 ****************************/

/**
 * @brief 信号量构造函数
 *
 * @param count
 */
Semaphore::Semaphore(uint32_t count) {
    if (sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

/**
 * @brief 信号量析构函数
 *
 */
Semaphore::~Semaphore() { sem_destroy(&m_semaphore); }

/**
 * @brief P 操作 -1
 *
 */
void Semaphore::wait() {
    // P 操作 -1
    if (sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

/**
 * @brief V 操作 +1
 *
 */
void Semaphore::notify() {
    // V 操作 +1
    if (sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}

/****************************
 * p_thread 线程的封装
 ****************************/

/**
 * @brief 获取当前线程的实例对象
 *
 * static 一个当前线程只有一个实例对象
 *
 * @return Thread*
 */
Thread *Thread::GetThis() { return t_thread; }

/**
 * @brief 获取当前线程的名称
 *
 * static 一个当前线程只有一个名称
 *
 * @return const std::string&
 */
const std::string &Thread::GetName() { return t_thread_name; }

/**
 * @brief 设置当前线程的名称
 *
 * static
 *
 * @param name 线程名称
 */
void Thread::SetName(const std::string &name) {
    if (t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

/**
 * @brief 线程启动函数
 *
 * static 一个当前线程只有一个执行函数
 *
 * @param arg 函数参数
 * @return void*
 */
void *Thread::run(void *arg) {
    // Thread 实例的 this 指针作为参数 arg 传入
    Thread *thread = (Thread *)arg;
    // 设置当前线程实例的 id
    thread->m_id = ljrserver::GetThreadId();

    // 设置当前线程对象 static thread_local
    t_thread = thread;
    // 设置当前线程名称 static thread_local
    t_thread_name = thread->m_name;

    // 设置线程名称
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    // 取出执行函数 作为局部函数执行
    std::function<void()> cb;
    cb.swap(thread->m_cb);

    // 信号量 -1
    thread->m_semaphore.notify();

    // 开始执行线程函数
    cb();
    return 0;
}

/**
 * @brief 线程构造函数
 *
 * @param cb 执行函数
 * @param name 线程名称
 */
Thread::Thread(std::function<void()> cb, const std::string &name)
    : m_cb(cb), m_name(name) {
    // 是否有线程名称
    if (name.empty()) {
        // 线程名称默认 "UNKNOW"
        m_name = "UNKNOW";
    }

    // 创建线程 将当前线程 Thread 实例对象作为参数传入最后一个参数 this
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt) {
        // 返回值不等于 0 创建线程失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "pthread_create thread fail, rt = " << rt << " name = " << name;
        // 抛出异常
        throw std::logic_error("pthread_create error");
    }

    // 信号量 +1 等待线程启动函数的执行
    m_semaphore.wait();
    // 等线程函数执行起来
}

/**
 * @brief 线程析构函数
 *
 */
Thread::~Thread() {
    if (m_thread) {
        // 非阻塞
        pthread_detach(m_thread);
    }
}

/**
 * @brief 线程 join
 *
 */
void Thread::join() {
    if (m_thread) {
        // 阻塞
        int rt = pthread_join(m_thread, nullptr);
        if (rt) {
            LJRSERVER_LOG_ERROR(g_logger)
                << "pthread_join thread fail, rt = " << rt
                << " name = " << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

}  // namespace ljrserver
