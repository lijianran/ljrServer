
#ifndef __LJRSERVER_ADDRESS_H__
#define __LJRSERVER_ADDRESS_H__

#include <memory>
#include <string>
// socket
#include <sys/socket.h>
#include <sys/types.h>
// Unix地址
#include <sys/un.h>
// IP地址
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

#include <vector>
#include <map>

namespace ljrserver
{

    class IPAddress;

    // 网络地址虚基类
    class Address
    {
    public:
        typedef std::shared_ptr<Address> ptr;

        static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);

        static bool Lookup(std::vector<Address::ptr> &result, const std::string &host,
                           int family = AF_INET, int type = 0, int protocol = 0);

        static Address::ptr LookupAny(const std::string &host, int family = AF_INET,
                                      int type = 0, int protocol = 0);

        static std::shared_ptr<IPAddress> LookupAnyIPAdress(const std::string &host, int family = AF_INET,
                                                         int type = 0, int protocol = 0);

        static bool GetInterfaceAddresses(std::multimap<std::string,
                                                        std::pair<Address::ptr, uint32_t>> &result,
                                          int family = AF_INET);

        static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result,
                                          const std::string &iface, int family = AF_INET);

        // 虚基类的析构函数写成了 virtual ~Address(); 报错
        virtual ~Address() {}

        int getFamily() const;

        virtual const sockaddr *getAddr() const = 0;

        virtual sockaddr *getAddr() = 0;

        virtual socklen_t getAddrLen() const = 0;

        virtual std::ostream &insert(std::ostream &os) const = 0;

        std::string toString() const;

        bool operator<(const Address &rhs) const;

        bool operator==(const Address &rhs) const;

        bool operator!=(const Address &rhs) const;
    };

    // IP地址
    class IPAddress : public Address
    {
    public:
        typedef std::shared_ptr<IPAddress> ptr;
        //
        static IPAddress::ptr Create(const char *address, uint16_t port = 0);
        // 广播地址
        virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
        // 网段地址
        virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
        // 子网地址
        virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;
        // 端口号
        virtual uint32_t getPort() const = 0;
        //
        virtual void setPort(uint16_t v) = 0;
    };

    // IPv4地址
    class IPv4Address : public IPAddress
    {
    public:
        typedef std::shared_ptr<IPv4Address> ptr;

        static IPv4Address::ptr Create(const char *address, uint16_t port);

        IPv4Address(const sockaddr_in &address);

        IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        std::ostream &insert(std::ostream &os) const override;
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
        sockaddr_in m_addr;
    };

    // IPv6地址
    class IPv6Address : public IPAddress
    {
    public:
        typedef std::shared_ptr<IPv6Address> ptr;

        static IPv6Address::ptr Create(const char *address, uint16_t port = 0);

        IPv6Address();

        IPv6Address(const sockaddr_in6 &address);

        IPv6Address(const uint8_t address[16], uint16_t port = 0);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        std::ostream &insert(std::ostream &os) const override;
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
        sockaddr_in6 m_addr;
    };

    // Unix地址
    class UnixAddress : public Address
    {
    public:
        typedef std::shared_ptr<UnixAddress> ptr;

        UnixAddress();

        UnixAddress(const std::string &path);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        void setAddrLen(uint32_t v);

        std::ostream &insert(std::ostream &os) const override;

    private:
        sockaddr_un m_addr;

        socklen_t m_length;
    };

    // 未知地址
    class UnkonwnAddress : public Address
    {
    public:
        typedef std::shared_ptr<UnkonwnAddress> ptr;

        UnkonwnAddress(int family);

        UnkonwnAddress(const sockaddr &addr);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        std::ostream &insert(std::ostream &os) const override;

    private:
        sockaddr m_addr;
    };

    std::ostream &operator<<(std::ostream &os, Address &addr);

} // namespace ljrserver

#endif //__LJRSERVER_ADDRESS_H__