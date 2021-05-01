
#ifndef __LJRSERVER_FD_MANAGER_H__
#define __LJRSERVER_FD_MANAGER_H__

#include <memory>
#include <vector>
#include "thread.h"
#include "iomanager.h"
#include "singleton.h"

namespace ljrserver
{

    class FdCtx : public std::enable_shared_from_this<FdCtx>
    {
    public:
        typedef std::shared_ptr<FdCtx> ptr;

        FdCtx(int fd);

        ~FdCtx();

        bool init();
        bool isInit() const { return m_isInit; }
        bool isSocket() const { return m_isSocket; }
        bool isClosed() const { return m_isClosed; }
        // bool close();

        void setUserNonblock(bool v) { m_userNonblock = v; }
        bool getUserNonblock() const { return m_userNonblock; }

        void setSysNonblock(bool v) { m_sysNonblock = v; }
        bool getSysNonblock() const { return m_sysNonblock; }

        void setTimeout(int type, uint64_t v);
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
        // // 文件IO管理器
        // ljrserver::IOManager *m_manager;
    };

    class FdManager
    {
    public:
        typedef RWMutex RWmutexType;

        FdManager();

        FdCtx::ptr get(int fd, bool auto_create = false);

        void del(int fd);

    private:
        RWmutexType m_mutex;

        std::vector<FdCtx::ptr> m_datas;
    };

    typedef Singleton<FdManager> FdMgr;

} // namespace ljrserver

#endif //__LJRSERVER_FD_MANAGER_H__