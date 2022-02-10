
#include "fd_manager.h"
#include "hook.h"

#include <sys/types.h>

// stat  fstat  S_ISSOCK
#include <sys/stat.h>

#include <unistd.h>

namespace ljrserver {

/**
 * @brief 句柄构造函数
 *
 * @param fd
 */
FdCtx::FdCtx(int fd)
    : m_isInit(false),
      m_isSocket(false),
      m_sysNonblock(false),
      m_userNonblock(false),
      m_isClosed(false),
      m_fd(fd),
      m_recvTimeout(-1),
      m_sendTimeout(-1) {
    // 初始化列表要和成员变量声明顺序一致
    // 初始化
    init();
}

/**
 * @brief 句柄初始化
 *
 * @return true
 * @return false
 */
bool FdCtx::init() {
    // 是否初始化
    if (m_isInit) {
        // 已经初始化
        return true;
    }

    // 默认超时 -1 永久
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    if (-1 == fstat(m_fd, &fd_stat)) {
        // 句柄还没有初始化
        m_isInit = false;
        // 不是 socket
        m_isSocket = false;
    } else {
        // 句柄已经初始化成功
        m_isInit = true;
        // 是否是 socket 句柄
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    // 是否是 socket
    if (m_isSocket) {
        // 获取 flag
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        // 不是非阻塞
        if (!(flags & O_NONBLOCK)) {
            // 设置非阻塞
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }

        // 系统非阻塞
        m_sysNonblock = true;
    } else {
        // 系统阻塞
        m_sysNonblock = false;
    }

    // 用户阻塞
    m_userNonblock = false;
    // 句柄没有关闭
    m_isClosed = false;

    // 返回是否初始化
    return m_isInit;
}

/**
 * @brief 句柄析构函数
 *
 */
FdCtx::~FdCtx() {}

/**
 * @brief 设置超时
 *
 * @param type 发送 / 接收
 * @param v 超时时间
 */
void FdCtx::setTimeout(int type, uint64_t v) {
    if (type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

/**
 * @brief 获取超时
 *
 * @param type 发送 / 接收
 * @return uint64_t 超时时间
 */
uint64_t FdCtx::getTimeout(int type) {
    if (type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

/**
 * @brief 句柄管理器的构造函数
 *
 */
FdManager::FdManager() {
    // 初始化句柄数组大小
    m_datas.resize(64);
}

/**
 * @brief 获取句柄对象
 *
 * @param fd 句柄
 * @param auto_create 不存在是否创建 [= false]
 * @return FdCtx::ptr
 */
FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if (fd == -1) {
        return nullptr;
    }

    // 上读锁
    RWmutexType::ReadLock lock(m_mutex);
    if ((int)m_datas.size() < fd) {
        // 没有句柄
        if (auto_create == false) {
            // 不自动创建
            return nullptr;
        }
        // 自动创建
    } else {
        // 有句柄 不自动创建
        if (m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    // 解读锁
    lock.unlock();

    // 上写锁
    RWmutexType::WriteLock lock2(m_mutex);
    // 创建新句柄对象
    FdCtx::ptr ctx(new FdCtx(fd));

    // 是否需要扩容
    if (fd >= (int)m_datas.size()) {
        // 1.5 倍扩容
        m_datas.resize(fd * 1.5);
    }

    // 加入句柄对象数组
    m_datas[fd] = ctx;

    return ctx;
}

/**
 * @brief 删除句柄
 *
 * @param fd 句柄
 */
void FdManager::del(int fd) {
    // 上写锁
    RWmutexType::WriteLock lock(m_mutex);

    if ((int)m_datas.size() <= fd) {
        // 没有句柄
        return;
    }

    // 清空句柄
    m_datas[fd].reset();
}

}  // namespace ljrserver
