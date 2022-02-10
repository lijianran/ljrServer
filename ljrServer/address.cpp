
#include "address.h"
#include "endian.h"
#include "log.h"

// string stream 字符串流
#include <sstream>
// get addrinfo
#include <netdb.h>
// get 网卡信息
#include <ifaddrs.h>
// offsetof
#include <stddef.h>

namespace ljrserver {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/**
 * @brief 子网掩码
 *
 * @tparam T
 * @param bits
 * @return T
 */
template <class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

/**
 * @brief 模版函数 统计 1 的位数
 *
 * @tparam T
 * @param value
 * @return uint32_t
 */
template <class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for (; value; ++result) {
        value &= value - 1;
    }
    return result;
}

/***************
Address 抽象类
***************/

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
bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family, int type, int protocol) {
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char *service = NULL;

    // 检查 ipv6 地址  service
    if (!host.empty() && host[0] == '[') {
        // 查找 ]
        const char *endipv6 =
            (const char *)memchr(host.c_str() + 1, ']', host.size() - 1);

        if (endipv6) {
            // TODO: check out of range
            if (*(endipv6 + 1) == ':') {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    // 检查 node service
    if (node.empty()) {
        // 查找 :
        service = (const char *)memchr(host.c_str(), ':', host.size());
        if (service) {
            if (!memchr(service + 1, ':',
                        host.c_str() + host.size() - service - 1)) {
                // 没有 : 了
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    if (node.empty()) {
        node = host;
    }

    // 获取 node:service
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if (error) {
        // 获取失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "Address::Lookup getaddress(" << host << ", " << family << ", "
            << type << ") error = " << error << " errno = " << errno
            << " error-string = " << strerror(error);
        return false;
    }

    // 遍历结果
    next = results;
    while (next) {
        // 加入地址结果列表
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        // 下一个地址结果
        next = next->ai_next;
    }

    // 释放内存
    freeaddrinfo(results);
    return true;
}

/**
 * @brief 通过 host 地址返回任意一个 Address
 *
 * @param host 域名 服务器名等 举例: www.baidu.com[:80] (方括号为可选内容)
 * @param family 协议族 (AF_INET, AF_INET6, AF_UNIX)
 * @param type socket 类型 SOCK_STREAM、SOCK_DGRAM 等
 * @param protocol 协议 IPPROTO_TCP、IPPROTO_UDP 等
 * @return Address::ptr
 */
Address::ptr Address::LookupAny(const std::string &host, int family, int type,
                                int protocol) {
    // 调用 Lookup 查找结果地址列表
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        // 直接返回第一个结果
        return result[0];
    }
    return nullptr;
}

/**
 * @brief 通过 host 地址返回任意一个 IPAddress
 *
 * @param host 域名 服务器名等 举例: www.baidu.com[:80] (方括号为可选内容)
 * @param family 协议族 (AF_INET, AF_INET6, AF_UNIX)
 * @param type socket 类型 SOCK_STREAM、SOCK_DGRAM 等
 * @param protocol 协议 IPPROTO_TCP、IPPROTO_UDP 等
 * @return std::shared_ptr<IPAddress>
 */
IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host, int family,
                                           int type, int protocol) {
    // 调用 Lookup 查找结果地址列表
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        // 遍历地址结果
        for (auto &i : result) {
            // 尝试动态指针转换
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) {
                // 转换成功就返回
                return v;
            }
        }
    }
    return nullptr;
}

/**
 * @brief 返回本机所有网卡的 <网卡名, 地址, 子网掩码位数>
 *
 * @param result 保存本机所有地址
 * @param family 协议族 (AF_INET, AF_INET6, AF_UNIX) [= AF_INET]
 * @return true
 * @return false
 */
bool Address::GetInterfaceAddresses(
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
    int family) {
    // 网卡信息
    struct ifaddrs *next, *results;
    // 获取信息
    if (getifaddrs(&results) != 0) {
        // 获取失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "Address::GetInterfaceAddresses getifaddrs error = " << errno
            << " error-string = " << strerror(errno);
        return false;
    }

    try {
        // 遍历结果信息
        for (next = results; next; next = next->ifa_next) {
            // 基类指针
            Address::ptr addr;
            uint32_t prefix_len = ~0u;

            // 检查协议簇
            if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }

            // 协议簇
            switch (next->ifa_addr->sa_family) {
                case AF_INET: {
                    // IPv4
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                    // 子网掩码
                    uint32_t netmask =
                        ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
                    // 掩码位数
                    prefix_len = CountBytes(netmask);

                } break;
                case AF_INET6: {
                    // IPv6
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    // 子网掩码
                    in6_addr &netmask =
                        ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
                    // 掩码位数
                    prefix_len = 0;
                    for (int i = 0; i < 16; ++i) {
                        prefix_len += CountBytes(netmask.s6_addr[i]);
                    }

                } break;
                default:
                    break;
            }

            if (addr) {
                // 插入结果 <网卡名, 地址, 子网掩码位数>
                result.insert(std::make_pair(next->ifa_name,
                                             std::make_pair(addr, prefix_len)));
            }
        }

    } catch (const std::exception &e) {
        // 错误
        LJRSERVER_LOG_ERROR(g_logger)
            << "Address::GetInterfaceAddresses exception: " << e.what();
        // 释放内存
        freeifaddrs(results);

        return false;
    }

    // 释放内存
    freeifaddrs(results);

    return true;
}

/**
 * @brief 获取指定网卡的地址和子网掩码位数
 *
 * @param result 保存指定网卡所有地址
 * @param iface 网卡名称
 * @param family 协议族 (AF_INET, AF_INET6, AF_UNIX) [= AF_INET]
 * @return true
 * @return false
 */
bool Address::GetInterfaceAddresses(
    std::vector<std::pair<Address::ptr, uint32_t>> &result,
    const std::string &iface, int family) {
    // 网卡名称为空或者 "*"
    if (iface.empty() || iface == "*") {
        if (family == AF_INET || family == AF_UNSPEC) {
            // IPv4
            result.push_back(
                std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if (family == AF_INET6 || family == AF_UNSPEC) {
            // IPv6
            result.push_back(
                std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    // 所有网卡结果
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;

    // 调用 GetInterfaceAddresses 重载函数
    if (!GetInterfaceAddresses(results, family)) {
        return false;
    }

    // 查找 key 名称
    auto its = results.equal_range(iface);
    for (; its.first != its.second; ++its.first) {
        // std::pair<Address::ptr, uint32_t> 加入结果
        result.push_back(its.first->second);
    }

    return true;
}

/**
 * @brief 通过 sockaddr 指针创建 Address
 *
 * @param addr sockaddr 指针
 * @param addrlen sockaddr 的长度
 * @return Address::ptr 返回和 sockaddr 相匹配的 Address 失败返回 nullptr
 */
Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
    if (addr == nullptr) {
        return nullptr;
    }
    // 基类指针 多态
    Address::ptr result;
    // 协议簇
    switch (addr->sa_family) {
        case AF_INET:
            // IPv4
            result.reset(new IPv4Address(*(const sockaddr_in *)addr));
            break;
        case AF_INET6:
            // IPv6
            result.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
            break;
        default:
            // Unknow
            result.reset(new UnkonwnAddress(*addr));
            break;
    }
    // 返回基类指针
    return result;
}

/**
 * @brief 获取协议簇 (AF_INET, AF_INET6, AF_UNIX)
 *
 * @return int
 */
int Address::getFamily() const {
    // 基类指针 动态多态
    return getAddr()->sa_family;
}

/**
 * @brief 返回地址的可读性字符串
 *
 * @return std::string
 */
std::string Address::toString() const {
    std::stringstream ss;

    // 动态多态 向流写入数据
    insert(ss);

    return ss.str();
}

/**
 * @brief 重载小于号比较函数
 */
bool Address::operator<(const Address &rhs) const {
    // 先比较最小长度
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    // 字符串比较
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);

    if (result < 0) {
        // 小于
        return true;
    } else if (result > 0) {
        // 大于
        return false;
    } else if (getAddrLen() < rhs.getAddrLen()) {
        // 等于 比较长度
        return true;
    }
    return false;
}

/**
 * @brief 重载等于函数
 */
bool Address::operator==(const Address &rhs) const {
    // 长度相等且比较字符串相等
    return getAddrLen() == rhs.getAddrLen() &&
           memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

/**
 * @brief 重载不等于函数
 */
bool Address::operator!=(const Address &rhs) const {
    // 直接比较对象
    return !(*this == rhs);
}

/***************
IPAddress
***************/

/**
 * @brief 通过域名 IP 服务器名创建 IPAddress
 *
 * @param address 域名 IP 服务器名等 举例: www.baidu.com
 * @param port 端口号 [= 0]
 * @return IPAddress::ptr 调用成功返回 IPAddress 失败返回 nullptr
 */
IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
    // 地址信息
    addrinfo hints, *results;
    // 清零
    memset(&hints, 0, sizeof(hints));

    // 解析 字符/数字
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    // 获取地址信息
    int error = getaddrinfo(address, NULL, &hints, &results);
    if (error) {
        // 获取失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "IPAddress::Create(" << address << ", " << port
            << ") error=" << error << " errno=" << errno
            << " error-string=" << strerror(errno);
        return nullptr;
    }

    try {
        // 调用基类 Create 然后从 Address 动态转换至 IPAddress
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
            Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));

        if (result) {
            // 基类指针 动态多态
            result->setPort(port);
        }
        // 清理内存
        freeaddrinfo(results);
        return result;

    } catch (const std::exception &e) {
        // 出现异常 清理内存
        freeaddrinfo(results);
        return nullptr;
    }
}

/***************
IPv4 地址
***************/

/**
 * @brief 使用点分十进制地址创建 IPv4Address
 *
 * @param address 点分十进制地址 如:192.168.1.1
 * @param port 端口号
 * @return IPv4Address::ptr 成功返回 IPv4Address 失败返回 nullptr
 */
IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port) {
    // 转换文本型的地址
    IPv4Address::ptr rt(new IPv4Address);

    // 小端
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    // 二进制网络字节序 -> rt->m_addr.sin_addr
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);

    if (result <= 0) {
        // 转换失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "IPv4Address::Create(" << address << ", " << port
            << ") rt = " << result << " errno = " << errno
            << " errno-string = " << strerror(errno);
        return nullptr;
    }

    // 成功返回 IPv4Address
    return rt;
}

/**
 * @brief 通过 sockaddr_in 构造 IPv4Address
 *
 * @param address sockaddr_in 结构体
 */
IPv4Address::IPv4Address(const sockaddr_in &address) { m_addr = address; }

/**
 * @brief 通过二进制地址构造 IPv4Address
 * @param[in] address 二进制地址 address
 * @param[in] port 端口号
 */
IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    // 清空
    memset(&m_addr, 0, sizeof(m_addr));
    // 协议簇 IPv4
    m_addr.sin_family = AF_INET;
    // 小端 端口号
    m_addr.sin_port = byteswapOnLittleEndian(port);
    // 小端 地址
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

/**
 * 实现 Address 虚函数
 */

const sockaddr *IPv4Address::getAddr() const { return (sockaddr *)&m_addr; }

sockaddr *IPv4Address::getAddr() { return (sockaddr *)&m_addr; }

socklen_t IPv4Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream &IPv4Address::insert(std::ostream &os) const {
    // 小端 地址
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);

    // 输出点分十进制地址
    os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "." << (addr & 0xff);

    // 小端 端口号
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);

    return os;
}

/**
 * 实现 IPAddress 虚函数
 */

// 广播地址
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if (prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |=
        byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

// 网段地址
IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {
    if (prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &=
        byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

// 子网地址
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr =
        ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

// 获得端口号
uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

// 设置端口号
void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}

/***************
IPv6 地址
***************/

/**
 * @brief 通过 IPv6 地址字符串构造 IPv6Address
 *
 * @param address IPv6 地址字符串
 * @param port 端口号
 * @return IPv6Address::ptr
 */
IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port) {
    IPv6Address::ptr rt(new IPv6Address);
    // 小端
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    // 二进制网络字节序 -> rt->m_addr.sin6_addr
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);

    if (result <= 0) {
        // 转换失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "IPv6Address::Create(" << address << ", " << port
            << ") rt = " << result << " errno = " << errno
            << " errno-string = " << strerror(errno);
        return nullptr;
    }

    // 返回地址 IPv6Address
    return rt;
}

/**
 * @brief 默认构造函数
 *
 */
IPv6Address::IPv6Address() {
    // 清空 IPv6 地址结构体
    memset(&m_addr, 0, sizeof(m_addr));
    // 协议簇 IPv6
    m_addr.sin6_family = AF_INET6;
}

/**
 * @brief 通过 sockaddr_in6 构造 IPv6Address
 *
 * @param address sockaddr_in6 结构体
 */
IPv6Address::IPv6Address(const sockaddr_in6 &address) { m_addr = address; }

/**
 * @brief 通过 IPv6 二进制地址构造 IPv6Address
 *
 * @param address IPv6 二进制地址
 * @param port 端口号
 */
IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
    // 清空
    memset(&m_addr, 0, sizeof(m_addr));
    // 协议簇 IPv6
    m_addr.sin6_family = AF_INET6;
    // 小端 端口号
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    // 地址结构体
    memcpy(&m_addr.sin6_addr, address, 16);
    // m_addr.sin6_addr.s6_addr = byteswapOnLittleEndian(address);
}

/**
 * 实现 Address 虚函数
 */
const sockaddr *IPv6Address::getAddr() const { return (sockaddr *)&m_addr; }

sockaddr *IPv6Address::getAddr() { return (sockaddr *)&m_addr; }

socklen_t IPv6Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream &IPv6Address::insert(std::ostream &os) const {
    os << "[";
    uint16_t *addr = (uint16_t *)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for (size_t i = 0; i < 8; ++i) {
        if (addr[i] == 0 && !used_zeros) {
            continue;
        }
        if (i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if (i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    if (!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

/**
 * 实现 IPAddress 虚函数
 */

// 广播地址
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for (int i = prefix_len / 8; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

// 网段地址
IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    // for (int i = prefix_len / 8; i < 16; ++i)
    // {
    //     baddr.sin6_addr.s6_addr[i] = 0xff;
    // }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

// 子网地址
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len / 8] =
        ~CreateMask<uint32_t>(prefix_len % 8);

    for (uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }

    return IPv6Address::ptr(new IPv6Address(subnet));
}

// 获得端口号
uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

// 设置端口号
void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

/***************
Unix 地址
***************/

// 地址路径最大长度
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

/**
 * @brief 默认构造函数
 */
UnixAddress::UnixAddress() {
    // 清零
    memset(&m_addr, 0, sizeof(m_addr));
    // 协议簇
    m_addr.sun_family = AF_UNIX;
    // 地址长度
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

/**
 * @brief 通过路径构造 UnixAddress
 *
 * @param path UnixSocket 路径 (长度小于 MAX_PATH_LEN)
 */
UnixAddress::UnixAddress(const std::string &path) {
    // 清零
    memset(&m_addr, 0, sizeof(m_addr));
    // 协议簇
    m_addr.sun_family = AF_UNIX;

    // 地址长度
    // 写错 m_length = path.size() - 1;
    m_length = path.size() + 1;

    if (!path.empty() && path[0] == '\0') {
        --m_length;
    }

    // 写错 if (m_length <= sizeof(m_addr.sun_path))
    if (m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }

    // 复制地址数据
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    // 地址长度
    m_length += offsetof(sockaddr_un, sun_path);
}

/**
 * 实现 Address 虚函数
 */
const sockaddr *UnixAddress::getAddr() const { return (sockaddr *)&m_addr; }

sockaddr *UnixAddress::getAddr() { return (sockaddr *)&m_addr; }

socklen_t UnixAddress::getAddrLen() const { return m_length; }

void UnixAddress::setAddrLen(uint32_t v) { m_length = v; }

std::ostream &UnixAddress::insert(std::ostream &os) const {
    if (m_length > offsetof(sockaddr_un, sun_path) &&
        m_addr.sun_path[0] == '\0') {
        return os << "\\0"
                  << std::string(
                         m_addr.sun_path + 1,
                         m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

/***************
未知地址
***************/

/**
 * @brief 重载构造函数
 *
 * @param family 协议簇
 */
UnkonwnAddress::UnkonwnAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

/**
 * @brief 重载构造函数
 *
 * @param addr 地址结构体
 */
UnkonwnAddress::UnkonwnAddress(const sockaddr &addr) { m_addr = addr; }

/**
 * 实现 Address 虚函数
 */
const sockaddr *UnkonwnAddress::getAddr() const { return (sockaddr *)&m_addr; }

sockaddr *UnkonwnAddress::getAddr() { return (sockaddr *)&m_addr; }

socklen_t UnkonwnAddress::getAddrLen() const { return sizeof(m_addr); }

std::ostream &UnkonwnAddress::insert(std::ostream &os) const {
    os << "[UnknowAddress family = " << m_addr.sa_family << "]";
    return os;
}

/**
 * @brief 流式输出地址
 *
 * @param os 数据流
 * @param addr 地址对象 Address
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, Address &addr) {
    // Address 虚基类指针 动态多态
    return addr.insert(os);
}

}  // namespace ljrserver
