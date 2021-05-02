
#ifndef __LJRSERVER_ENDIAN_H__
#define __LJRSERVER_ENDIAN_H__

#define LJRSERVER_LITTLE_ENDIAN 1
#define LJRSERVER_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

namespace ljrserver
{

    // 64bit
    template <class T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_64((uint64_t)value);
    }

    // 32bit
    template <class T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_32((uint32_t)value);
    }

    // 16bit
    template <class T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_16((uint16_t)value);
    }

#if BYTE_ORDER == BIG_ENDIAN
#define LJRSERVER_BYTE_ORDER LJRSERVER_BIG_ENDIAN
#else
#define LJRSERVER_BYTE_ORDER LJRSERVER_LITTLE_ENDIAN
#endif

#if LJRSERVER_BYTE_ORDER == LJRSERVER_BIG_ENDIAN
    template <class T>
    T byteswapOnLittleEndian(T t)
    {
        return t;
    }

    template <class T>
    T byteswapOnBigEndian(T t)
    {
        return byteswap(t);
    }
#else
    template <class T>
    T byteswapOnLittleEndian(T t)
    {
        return byteswap(t);
    }

    template <class T>
    T byteswapOnBigEndian(T t)
    {
        return t;
    }
#endif

} // namespace ljrserver

#endif //__LJRSERVER_ENDIAN_H__