
#ifndef __LJRSERVER_ENDIAN_H__
#define __LJRSERVER_ENDIAN_H__

// 小端字节序
#define LJRSERVER_LITTLE_ENDIAN 1
// 大端字节序
#define LJRSERVER_BIG_ENDIAN 2

// bswap
#include <byteswap.h>
// int
#include <stdint.h>
// #include <type_traits>

namespace ljrserver {

/**
 * @brief 模版函数 8 字节类型的字节序转换
 * 64bit
 *
 * @tparam T
 * @param value
 * @return std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
 */
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteswap(
    T value) {
    return (T)bswap_64((uint64_t)value);
}

/**
 * @brief 模版函数 4 字节类型的字节序转换
 * 32bit
 *
 * @tparam T
 * @param value
 * @return std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
 */
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteswap(
    T value) {
    return (T)bswap_32((uint32_t)value);
}

/**
 * @brief 模版函数 2 字节类型的字节序转换
 * 16bit
 *
 * @tparam T
 * @param value
 * @return std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
 */
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteswap(
    T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
// 大端字节序
#define LJRSERVER_BYTE_ORDER LJRSERVER_BIG_ENDIAN
#else
// 小端字节序
#define LJRSERVER_BYTE_ORDER LJRSERVER_LITTLE_ENDIAN
#endif

#if LJRSERVER_BYTE_ORDER == LJRSERVER_BIG_ENDIAN
/**
 * @brief 只在小端机器上执行 byteswap 在大端机器上什么都不做
 *
 * @tparam T
 * @param t
 * @return T
 */
template <class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

/**
 * @brief 只在大端机器上执行 byteswap 在小端机器上什么都不做
 *
 * @tparam T
 * @param t
 * @return T
 */
template <class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

#else

/**
 * @brief 只在小端机器上执行 byteswap 在大端机器上什么都不做
 *
 * @tparam T
 * @param t
 * @return T
 */
template <class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

/**
 * @brief 只在大端机器上执行 byteswap 在小端机器上什么都不做
 * 
 * @tparam T 
 * @param t 
 * @return T 
 */
template <class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif

}  // namespace ljrserver

#endif  //__LJRSERVER_ENDIAN_H__