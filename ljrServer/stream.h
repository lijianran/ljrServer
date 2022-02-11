
#ifndef __LJRSERVER_STREAM_H__
#define __LJRSERVER_STREAM_H__

// 智能指针
#include <memory>
// 二进制字节数组
#include "bytearray.h"

namespace ljrserver {

/**
 * @brief Class 封装流的抽象类
 *
 * 将文件 socket 封装成统一的接口
 */
class Stream {
public:
    // 智能指针
    typedef std::shared_ptr<Stream> ptr;

    /**
     * @brief 虚基类的析构函数设置为虚函数
     *
     */
    virtual ~Stream() {}

    /**
     * @brief 关闭流
     *
     * 纯虚函数
     */
    virtual void close() = 0;

public:  /// 读取
    /**
     * @brief 读取数据到 buffer 纯虚函数
     *
     * @param buffer
     * @param length
     * @return int
     */
    virtual int read(void *buffer, size_t length) = 0;

    /**
     * @brief 读取数据到 byte array 纯虚函数
     *
     * @param ba
     * @param length
     * @return int
     */
    virtual int read(ByteArray::ptr ba, size_t length) = 0;

    /**
     * @brief 读取固定长度的数据到 buffer 虚函数
     *
     * @param buffer
     * @param length
     * @return int
     */
    virtual int readFixSize(void *buffer, size_t length);

    /**
     * @brief 读取固定长度的数据到 byte array 虚函数
     *
     * @param ba
     * @param length
     * @return int
     */
    virtual int readFixSize(ByteArray::ptr ba, size_t length);

public:  /// 写入
    /**
     * @brief 写入 buffer 中的数据 纯虚函数
     *
     * @param buffer
     * @param length
     * @return int
     */
    virtual int write(const void *buffer, size_t length) = 0;

    /**
     * @brief 写入 byte array 中的数据 纯虚函数
     *
     * @param ba
     * @param length
     * @return int
     */
    virtual int write(ByteArray::ptr ba, size_t length) = 0;

    /**
     * @brief 写入 buffer 中固定长度的数据  虚函数
     *
     * @param buffer
     * @param length
     * @return int
     */
    virtual int writeFixSize(const void *buffer, size_t length);

    /**
     * @brief 写入 byte array 固定长度的数据 虚函数
     *
     * @param ba
     * @param length
     * @return int
     */
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);
};

}  // namespace ljrserver

#endif  //__LJRSERVER_STREAM_H__