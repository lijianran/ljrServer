
#ifndef __LJRSERVER_ADDRESS_H__
#define __LJRSERVER_ADDRESS_H__

// 智能指针
#include <memory>
// STL
#include <string>
#include <vector>
#include <map>
// socket
#include <sys/socket.h>
#include <sys/types.h>
// Unix 地址
#include <sys/un.h>
// IP 地址
#include <arpa/inet.h>
#include <unistd.h>
// stream
#include <iostream>

namespace ljrserver {

class IPAddress;

/**
 * @brief 网络地址抽象类
 *
 */
class Address {
public:
    // 智能指针
    typedef std::shared_ptr<Address> ptr;

    /**
     * @brief 通过 sockaddr 指针创建 Address
     *
     * @param addr sockaddr 指针
     * @param addrlen sockaddr 的长度
     * @return Address::ptr 返回和 sockaddr 相匹配的 Address 失败返回 nullptr
     */
    static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);

    /**
     * @brief 通过 host 地址返回对应条件的所有 Address
     *
     * @param result 保存满足条件的 Address
     * @param host 域名 服务器名等 举例: www.baidu.com[:80] (方括号为可选内容)
     * @param family 协议族 (AF_INET, AF_INET6, AF_UNIX)
     * @param type socket 类型 SOCK_STREAM、SOCK_DGRAM 等
     * @param protocol 协议 IPPROTO_TCP、IPPROTO_UDP 等
     * @return true 转换成功
     * @return false 转换失败
     */
    static bool Lookup(std::vector<Address::ptr> &result,
                       const std::string &host, int family = AF_INET,
                       int type = 0, int protocol = 0);

    /**
     * @brief 通过 host 地址返回任意一个 Address
     *
     * @param host 域名 服务器名等 举例: www.baidu.com[:80] (方括号为可选内容)
     * @param family 协议族 (AF_INET, AF_INET6, AF_UNIX)
     * @param type socket 类型 SOCK_STREAM、SOCK_DGRAM 等
     * @param protocol 协议 IPPROTO_TCP、IPPROTO_UDP 等
     * @return Address::ptr
     */
    static Address::ptr LookupAny(const std::string &host, int family = AF_INET,
                                  int type = 0, int protocol = 0);

    /**
     * @brief 通过 host 地址返回任意一个 IPAddress
     *
     * @param host 域名 服务器名等 举例: www.baidu.com[:80] (方括号为可选内容)
     * @param family 协议族 (AF_INET, AF_INET6, AF_UNIX)
     * @param type socket 类型 SOCK_STREAM、SOCK_DGRAM 等
     * @param protocol 协议 IPPROTO_TCP、IPPROTO_UDP 等
     * @return std::shared_ptr<IPAddress>
     */
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(
        const std::string &host, int family = AF_INET, int type = 0,
        int protocol = 0);

    /**
     * @brief 返回本机所有网卡的 <网卡名, 地址, 子网掩码位数> [= AF_INET]
     *
     * @param result 保存本机所有地址
     * @param family 协议族 (AF_INET, AF_INET6, AF_UNIX)
     * @return true
     * @return false
     */
    static bool GetInterfaceAddresses(
        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
        int family = AF_INET);

    /**
     * @brief 获取指定网卡的地址和子网掩码位数
     *
     * @param result 保存指定网卡所有地址
     * @param iface 网卡名称
     * @param family 协议族 (AF_INET, AF_INET6, AF_UNIX) [= AF_INET]
     * @return true
     * @return false
     */
    static bool GetInterfaceAddresses(
        std::vector<std::pair<Address::ptr, uint32_t>> &result,
        const std::string &iface, int family = AF_INET);

    /**
     * @brief 地址基类析构函数 虚函数
     *
     * 虚基类的析构函数写成了 virtual ~Address(); 报错
     */
    virtual ~Address() {}

    /**
     * @brief 获取协议簇 (AF_INET, AF_INET6, AF_UNIX)
     *
     * @return int
     */
    int getFamily() const;

    /**
     * @brief 获取地址 纯虚函数 只读
     *
     * @return const sockaddr*
     */
    virtual const sockaddr *getAddr() const = 0;

    /**
     * @brief 获取地址 纯虚函数 可读写
     *
     * @return sockaddr*
     */
    virtual sockaddr *getAddr() = 0;

    /**
     * @brief 获取地址长度 纯虚函数
     *
     * @return socklen_t
     */
    virtual socklen_t getAddrLen() const = 0;

    /**
     * @brief 可读性输出地址
     *
     * @param os
     * @return std::ostream&
     */
    virtual std::ostream &insert(std::ostream &os) const = 0;

    /**
     * @brief 返回地址的可读性字符串
     *
     * @return std::string
     */
    std::string toString() const;

    /**
     * @brief 重载小于号比较函数
     */
    bool operator<(const Address &rhs) const;

    /**
     * @brief 重载等于函数
     */
    bool operator==(const Address &rhs) const;

    /**
     * @brief 重载不等于函数
     */
    bool operator!=(const Address &rhs) const;
};

/**
 * @brief Class 封装 IP 地址 抽象类
 *
 * 继承自 Address 抽象类
 */
class IPAddress : public Address {
public:
    // 智能指针
    typedef std::shared_ptr<IPAddress> ptr;

    /**
     * @brief 通过域名 IP 服务器名创建 IPAddress
     *
     * @param address 域名 IP 服务器名等 举例: www.baidu.com
     * @param port 端口号 [= 0]
     * @return IPAddress::ptr 调用成功返回 IPAddress 失败返回 nullptr
     */
    static IPAddress::ptr Create(const char *address, uint16_t port = 0);

    /**
     * 纯虚函数 由子类各自实现
     */
    // 广播地址
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    // 网段地址
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
    // 子网地址
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;
    // 端口号
    virtual uint32_t getPort() const = 0;
    // 设置端口号
    virtual void setPort(uint16_t v) = 0;
};

/**
 * @brief Class 封装 IPv4 地址
 *
 * 继承自 IPAddress
 */
class IPv4Address : public IPAddress {
public:
    // 智能指针
    typedef std::shared_ptr<IPv4Address> ptr;

    /**
     * @brief 使用点分十进制地址创建 IPv4Address
     *
     * @param address 点分十进制地址 如:192.168.1.1
     * @param port 端口号
     * @return IPv4Address::ptr 成功返回 IPv4Address 失败返回 nullptr
     */
    static IPv4Address::ptr Create(const char *address, uint16_t port);

    /**
     * @brief 通过 sockaddr_in 构造 IPv4Address
     *
     * @param address sockaddr_in 结构体
     */
    IPv4Address(const sockaddr_in &address);

    /**
     * @brief 通过二进制地址构造 IPv4Address
     * @param[in] address 二进制地址 address
     * @param[in] port 端口号
     */
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

public:
    /**
     * 实现 Address 虚函数
     */
    const sockaddr *getAddr() const override;

    sockaddr *getAddr() override;

    socklen_t getAddrLen() const override;

    std::ostream &insert(std::ostream &os) const override;

public:
    /**
     * 实现 IPAddress 虚函数
     */
    // 广播地址
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    // 网段地址
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    // 子网地址
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    // 获得端口号
    uint32_t getPort() const override;
    // 设置端口号
    void setPort(uint16_t v) override;

private:
    // 地址结构体
    sockaddr_in m_addr;
};

/**
 * @brief Class 封装 IPv6 地址
 *
 * 继承自 IPAddress
 */
class IPv6Address : public IPAddress {
public:
    // 智能指针
    typedef std::shared_ptr<IPv6Address> ptr;

    /**
     * @brief 通过 IPv6 地址字符串构造 IPv6Address
     *
     * @param address IPv6 地址字符串
     * @param port 端口号
     * @return IPv6Address::ptr
     */
    static IPv6Address::ptr Create(const char *address, uint16_t port = 0);

    /**
     * @brief 默认构造函数
     *
     */
    IPv6Address();

    /**
     * @brief 通过 sockaddr_in6 构造 IPv6Address
     *
     * @param address sockaddr_in6 结构体
     */
    IPv6Address(const sockaddr_in6 &address);

    /**
     * @brief 通过 IPv6 二进制地址构造 IPv6Address
     *
     * @param address IPv6 二进制地址
     * @param port 端口号
     */
    IPv6Address(const uint8_t address[16], uint16_t port = 0);

public:
    /**
     * 实现 Address 虚函数
     */
    const sockaddr *getAddr() const override;

    sockaddr *getAddr() override;

    socklen_t getAddrLen() const override;

    std::ostream &insert(std::ostream &os) const override;

public:
    /**
     * 实现 IPAddress 虚函数
     */
    // 广播地址
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    // 网段地址
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    // 子网地址
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    // 获得端口号
    uint32_t getPort() const override;
    // 设置端口号
    void setPort(uint16_t v) override;

private:
    // IPv6 地址结构体
    sockaddr_in6 m_addr;
};

/**
 * @brief Class 封装 Unix 地址
 *
 * 继承自 Address
 */
class UnixAddress : public Address {
public:
    // 智能指针
    typedef std::shared_ptr<UnixAddress> ptr;

    /**
     * @brief 默认构造函数
     */
    UnixAddress();

    /**
     * @brief 通过路径构造 UnixAddress
     *
     * @param path UnixSocket 路径 (长度小于 UNIX_PATH_MAX)
     */
    UnixAddress(const std::string &path);

public:
    /**
     * 实现 Address 虚函数
     */
    const sockaddr *getAddr() const override;

    sockaddr *getAddr() override;

    socklen_t getAddrLen() const override;

    void setAddrLen(uint32_t v);

    std::ostream &insert(std::ostream &os) const override;

private:
    // Unix 地址结构体
    sockaddr_un m_addr;

    // 地址长度
    socklen_t m_length;
};

/**
 * @brief Class 未知地址
 *
 * 继承自 Address
 */
class UnkonwnAddress : public Address {
public:
    // 智能指针
    typedef std::shared_ptr<UnkonwnAddress> ptr;

    /**
     * @brief 重载构造函数
     *
     * @param family 协议簇
     */
    UnkonwnAddress(int family);

    /**
     * @brief 重载构造函数
     *
     * @param addr 地址结构体
     */
    UnkonwnAddress(const sockaddr &addr);

public:
    /**
     * 实现 Address 虚函数
     */
    const sockaddr *getAddr() const override;

    sockaddr *getAddr() override;

    socklen_t getAddrLen() const override;

    std::ostream &insert(std::ostream &os) const override;

private:
    // 地址结构体
    sockaddr m_addr;
};

/**
 * @brief 流式输出地址
 *
 * @param os 数据流
 * @param addr 地址对象 Address
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, Address &addr);

}  // namespace ljrserver

#endif  //__LJRSERVER_ADDRESS_H__