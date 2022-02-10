
#ifndef __LJRSERVER_FD_MANAGER_H__
#define __LJRSERVER_FD_MANAGER_H__

// 智能指针
#include <memory>
// 动态数组
#include <vector>

// 线程
#include "thread.h"
// IO 管理
#include "iomanager.h"
// 单例模式
#include "singleton.h"

namespace ljrserver {

/**
 * @brief Class 句柄
 *
 */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    // 智能指针
    typedef std::shared_ptr<FdCtx> ptr;

    /**
     * @brief 句柄构造函数
     *
     * @param fd
     */
    FdCtx(int fd);

    /**
     * @brief 句柄析构函数
     *
     */
    ~FdCtx();

    /**
     * @brief 句柄初始化
     *
     * @return true
     * @return false
     */
    bool init();

    /**
     * @brief 是否初始化
     *
     * @return true
     * @return false
     */
    bool isInit() const { return m_isInit; }

    /**
     * @brief 是否是 socket 句柄
     *
     * @return true
     * @return false
     */
    bool isSocket() const { return m_isSocket; }

    /**
     * @brief 是否已经关闭
     *
     * @return true
     * @return false
     */
    bool isClosed() const { return m_isClosed; }
    // bool close();

    /**
     * @brief 设置用户是否非阻塞
     *
     * @param v
     */
    void setUserNonblock(bool v) { m_userNonblock = v; }

    /**
     * @brief 用户是否设置阻塞
     *
     * @return true
     * @return false
     */
    bool getUserNonblock() const { return m_userNonblock; }

    /**
     * @brief 设置系统是否非阻塞
     *
     * @param v
     */
    void setSysNonblock(bool v) { m_sysNonblock = v; }

    /**
     * @brief 系统是否阻塞
     * 
     * @return true 
     * @return false 
     */
    bool getSysNonblock() const { return m_sysNonblock; }

    /**
     * @brief 设置超时
     *
     * @param type 发送 / 接收
     * @param v 超时时间
     */
    void setTimeout(int type, uint64_t v);

    /**
     * @brief 获取超时
     *
     * @param type 发送 / 接收
     * @return uint64_t 超时时间
     */
    uint64_t getTimeout(int type);

private:
    // 是否初始化
    bool m_isInit : 1;

    // 是否socket句柄
    bool m_isSocket : 1;

    // 系统是否已经设置为非阻塞
    bool m_sysNonblock : 1;

    // 用户是否已经设置为非阻塞
    bool m_userNonblock : 1;

    // 是否已经关闭
    bool m_isClosed : 1;

    // 文件句柄
    int m_fd;

    // 发送超时时间
    uint64_t m_recvTimeout;

    // 接收超时时间
    uint64_t m_sendTimeout;

    // 文件IO管理器
    // ljrserver::IOManager *m_manager;
};

/**
 * @brief Class 句柄管理器
 *
 */
class FdManager {
public:
    // 读写锁
    typedef RWMutex RWmutexType;

    /**
     * @brief 句柄管理器的构造函数
     *
     */
    FdManager();

    /**
     * @brief 获取句柄对象
     *
     * @param fd 句柄
     * @param auto_create 不存在是否创建 [= false]
     * @return FdCtx::ptr
     */
    FdCtx::ptr get(int fd, bool auto_create = false);

    /**
     * @brief 删除句柄
     *
     * @param fd 句柄
     */
    void del(int fd);

private:
    // 读写锁
    RWmutexType m_mutex;

    // 句柄数组
    std::vector<FdCtx::ptr> m_datas;
};

// 单例模式
typedef Singleton<FdManager> FdMgr;

}  // namespace ljrserver

#endif  //__LJRSERVER_FD_MANAGER_H__