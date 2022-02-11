
#include "stream.h"

namespace ljrserver {

/**
 * @brief 读取固定长度的数据到 buffer 虚函数
 *
 * @param buffer
 * @param length
 * @return int
 */
int Stream::readFixSize(void *buffer, size_t length) {
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        size_t len = read((char *)buffer + offset, left);
        if (len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

/**
 * @brief 读取固定长度的数据到 byte array 虚函数
 *
 * @param ba
 * @param length
 * @return int
 */
int Stream::readFixSize(ByteArray::ptr ba, size_t length) {
    size_t left = length;
    while (left > 0) {
        size_t len = read(ba, left);
        if (len <= 0) {
            return len;
        }
        left -= len;
    }
    return length;
}

/**
 * @brief 写入 buffer 中固定长度的数据  虚函数
 *
 * @param buffer
 * @param length
 * @return int
 */
int Stream::writeFixSize(const void *buffer, size_t length) {
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        size_t len = write((const char *)buffer + offset, left);
        if (len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

/**
 * @brief 写入 byte array 固定长度的数据 虚函数
 *
 * @param ba
 * @param length
 * @return int
 */
int Stream::writeFixSize(ByteArray::ptr ba, size_t length) {
    size_t left = length;
    while (left > 0) {
        size_t len = write(ba, left);
        if (len <= 0) {
            return len;
        }
        left -= len;
    }
    return length;
}

}  // namespace ljrserver
