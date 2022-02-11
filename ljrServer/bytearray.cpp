
#include "bytearray.h"

// 大小端字节序
#include "endian.h"
// 日志
#include "log.h"
// 字符串
#include <string.h>
// #include <sstream>
// io << std::hex
#include <iomanip>

namespace ljrserver {

// 系统日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/**
 * @brief 数据节点构造函数 重载
 *
 * @param s
 */
ByteArray::Node::Node(size_t s) : ptr(new char[s]), size(s), next(nullptr) {}

/**
 * @brief 数据节点构造函数 默认
 *
 * 默认内存块大小为零
 */
ByteArray::Node::Node() : ptr(nullptr), size(0), next(nullptr) {}

/**
 * @brief 数据节点析构函数
 *
 */
ByteArray::Node::~Node() {
    if (ptr) {
        delete[] ptr;
    }
}

/**
 * @brief 构造函数
 *
 * 使用指定长度的内存块构造 ByteArray
 *
 * @param base_size 内存块大小
 */
ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size),
      m_position(0),
      m_capacity(base_size),
      m_size(0),
      m_endian(LJRSERVER_BIG_ENDIAN),
      m_root(new Node(base_size)),
      m_cur(m_root) {
    // 默认大端存储
}

/**
 * @brief 析构函数
 *
 */
ByteArray::~ByteArray() {
    // 清理内存
    Node *tmp = m_root;
    while (tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

/******************************
32bit 64bit 编码解码
******************************/

/**
 * @brief 32bit 编码
 *
 * @param v
 * @return uint32_t
 */
static uint32_t EncodeZigzag32(const int32_t &v) {
    if (v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

/**
 * @brief 32bit 解码
 *
 * @param v
 * @return int32_t
 */
static int32_t DecodeZigzag32(const uint32_t &v) { return (v >> 1) ^ -(v & 1); }

/**
 * @brief 64bit 编码
 *
 * @param v
 * @return uint64_t
 */
static uint64_t EncodeZigzag64(const uint64_t &v) {
    if (v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

/**
 * @brief 64bit 解码
 *
 * @param v
 * @return int64_t
 */
static int64_t DecodeZigzag64(const uint64_t &v) { return (v >> 1) ^ -(v & 1); }

/******************************
Write 写入
******************************/

/// 固定长度
/**
 * @brief 写入固定长度 int8 类型的数据
 *
 * @param value
 */
void ByteArray::writeFint8(int8_t value) { write(&value, sizeof(value)); }

/**
 * @brief 写入固定长度 uint8 类型的数据
 *
 * @param value
 */
void ByteArray::writeFuint8(uint8_t value) { write(&value, sizeof(value)); }

/**
 * @brief 写入固定长度 int16 类型的数据
 *
 * @param value
 */
void ByteArray::writeFint16(int16_t value) {
    if (m_endian != LJRSERVER_BYTE_ORDER) {
        // 转换字节序
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度 uint16 类型的数据
 *
 * @param value
 */
void ByteArray::writeFuint16(uint16_t value) {
    if (m_endian != LJRSERVER_BYTE_ORDER) {
        // 转换字节序
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度 int32 类型的数据
 *
 * @param value
 */
void ByteArray::writeFint32(int32_t value) {
    if (m_endian != LJRSERVER_BYTE_ORDER) {
        // 转换字节序
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度 uint32 类型的数据
 *
 * @param value
 */
void ByteArray::writeFuint32(uint32_t value) {
    if (m_endian != LJRSERVER_BYTE_ORDER) {
        // 转换字节序
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度 int64 类型的数据
 *
 * @param value
 */
void ByteArray::writeFint64(int64_t value) {
    if (m_endian != LJRSERVER_BYTE_ORDER) {
        // 转换字节序
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief 写入固定长度 uint64 类型的数据
 *
 * @param value
 */
void ByteArray::writeFuint64(uint64_t value) {
    if (m_endian != LJRSERVER_BYTE_ORDER) {
        // 转换字节序
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/// 变长
/**
 * @brief 写入有符号 Varint32 类型的数据
 *
 * @param value
 */
void ByteArray::writeInt32(int32_t value) {
    writeUint32(EncodeZigzag32(value));
}

/**
 * @brief 写入无符号 Varint32 类型的数据
 *
 * @param value
 */
void ByteArray::writeUint32(uint32_t value) {
    uint8_t tmp[5];
    uint8_t i = 0;
    while (value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;

    // 写入
    write(tmp, i);
}

/**
 * @brief 写入有符号 Varint64 类型的数据
 *
 * @param value
 */
void ByteArray::writeInt64(int64_t value) {
    writeUint64(EncodeZigzag64(value));
}

/**
 * @brief 写入无符号 Varint64 类型的数据
 *
 * @param value
 */
void ByteArray::writeUint64(uint64_t value) {
    uint8_t tmp[10];
    uint8_t i = 0;
    while (value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;

    // 写入
    write(tmp, i);
}

/// 浮点数
/**
 * @brief 写入 float 类型的数据
 *
 * @param value
 */
void ByteArray::writeFloat(float value) {
    uint32_t v;
    // 复制出来
    memcpy(&v, &value, sizeof(value));
    // 写入 32bit 无符号 int
    writeFuint32(v);
}

/**
 * @brief 写入 double 类型的数据
 *
 * @param value
 */
void ByteArray::writeDouble(double value) {
    uint64_t v;
    // 复制出来
    memcpy(&v, &value, sizeof(value));
    // 64bit 无符号 int
    writeFuint64(v);
}

/// 字符串
/**
 * @brief 写入 std::string 类型的数据 长度为 16bit 无符号 int
 *
 * length:int16   data
 *
 * @param value
 */
void ByteArray::writeStringF16(const std::string &value) {
    // 写入长度 16bit 无符号整型
    writeFuint16(value.size());
    // 写入数据
    write(value.c_str(), value.size());
}

/**
 * @brief 写入 std::string 类型的数据 长度为 32bit 无符号 int
 *
 * length:int32   data
 *
 * @param value
 */
void ByteArray::writeStringF32(const std::string &value) {
    // 写入长度 32bit 无符号整型
    writeFuint32(value.size());
    // 写入数据
    write(value.c_str(), value.size());
}

/**
 * @brief 写入 std::string 类型的数据 长度为 64bit 无符号 int
 *
 * length:int64   data
 *
 * @param value
 */
void ByteArray::writeStringF64(const std::string &value) {
    // 写入长度 64bit 无符号整型
    writeFuint64(value.size());
    // 写入数据
    write(value.c_str(), value.size());
}

/**
 * @brief 写入 std::string 类型的数据 长度为 64bit 无符号 int
 *
 * length:varint  data
 *
 * @param value
 */
void ByteArray::writeStringVint(const std::string &value) {
    // 变长写入长度 64bit 无符号整型
    writeUint64(value.size());
    // 写入数据
    write(value.c_str(), value.size());
}

/**
 * @brief 写入 std::string 类型的数据 不存储长度
 *
 * data
 *
 * @param value
 */
void ByteArray::writeStringWithoutLength(const std::string &value) {
    // 只写入数据
    write(value.c_str(), value.size());
}

/******************************
Read 读取
******************************/

/**
 * @brief 读取 int8 类型的数据
 *
 * @return int8_t
 */
int8_t ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

/**
 * @brief 读取 uint8 类型的数据
 *
 * @return uint8_t
 */
uint8_t ByteArray::readFuint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

/**
 * @brief 读取数据
 *
 * int16 uint16 int32 uint32 int64 uint64
 *
 * 判断大小端
 *
 */
#define XX(type)                            \
    type v;                                 \
    read(&v, sizeof(v));                    \
    if (m_endian == LJRSERVER_BYTE_ORDER) { \
        return v;                           \
    } else {                                \
        return byteswap(v);                 \
    }

int16_t ByteArray::readFint16() { XX(int16_t); }

uint16_t ByteArray::readFuint16() { XX(uint16_t); }

int32_t ByteArray::readFint32() { XX(int32_t); }

uint32_t ByteArray::readFuint32() { XX(uint32_t); }

int64_t ByteArray::readFint64() { XX(int64_t); }

uint64_t ByteArray::readFuint64() { XX(uint64_t); }
#undef XX

/// 变长
/**
 * @brief 读取有符号 Varint32 类型的数据
 *
 * @return int32_t
 */
int32_t ByteArray::readInt32() { return DecodeZigzag32(readUint32()); }

/**
 * @brief 读取无符号 Varint32 类型的数据
 *
 * @return uint32_t
 */
uint32_t ByteArray::readUint32() {
    uint32_t result = 0;
    for (int i = 0; i < 32; i += 7) {
        uint8_t b = readFint8();
        if (b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= (((uint32_t)(b & 0x7F)) << i);
        }
    }
    return result;
}

/**
 * @brief 读取有符号 Varint64 类型的数据
 *
 * @return int64_t
 */
int64_t ByteArray::readInt64() { return DecodeZigzag64(readUint64()); }

/**
 * @brief 读取无符号 Varint64 类型的数据
 *
 * @return uint64_t
 */
uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for (int i = 0; i < 32; i += 7) {
        uint8_t b = readFint8();
        if (b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7F)) << i);
        }
    }
    return result;
}

/// 浮点数
/**
 * @brief 读取 float 类型的数据
 *
 * @return float
 */
float ByteArray::readFloat() {
    // 读取 32bit 数据
    uint32_t v = readFuint32();
    // 转为 float
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

/**
 * @brief 读取 double 类型的数据
 *
 * @return double
 */
double ByteArray::readDouble() {
    // 读取 64bit 数据
    uint64_t v = readFuint64();
    // 转为 double
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

/// 字符串
/**
 * @brief 读取 std::string 类型的数据 长度为 uint16_t
 *
 * length:uint16    data
 *
 * @return std::string
 */
std::string ByteArray::readStringF16() {
    // 读取长度 uint16
    uint16_t len = readFuint16();

    // 缓存
    std::string buff;
    buff.resize(len);

    // 读取
    read(&buff[0], len);
    return buff;
}

/**
 * @brief 读取长度为 uint32_t 的 std::string 类型的数据
 *
 * length:uint32    data
 *
 * @return std::string
 */
std::string ByteArray::readStringF32() {
    // 读取长度 uint32
    uint32_t len = readFuint32();

    // 缓存
    std::string buff;
    buff.resize(len);

    // 读取
    read(&buff[0], len);
    return buff;
}

/**
 * @brief 读取长度为 uint64_t 的 std::string 类型的数据
 *
 * length:uint64    data
 *
 * @return std::string
 */
std::string ByteArray::readStringF64() {
    // 读取长度 uint64
    uint64_t len = readFuint64();

    // 缓存
    std::string buff;
    buff.resize(len);

    // 读取
    read(&buff[0], len);
    return buff;
}

/**
 * @brief 读取长度为 uint64_t 的变长 std::string 类型的数据
 *
 * length:varint   data
 *
 * @return std::string
 */
std::string ByteArray::readStringVint() {
    // 读取长度 uint64 变长
    uint64_t len = readUint64();

    // 缓存
    std::string buff;
    buff.resize(len);

    // 读取
    read(&buff[0], len);
    return buff;
}

/******************************
内部操作
******************************/

/**
 * @brief 清空数据
 *
 */
void ByteArray::clear() {
    // 位置 容量 清零
    m_position = m_size = 0;
    // 容量等于基础内存块大小 4kb
    m_capacity = m_baseSize;

    // 清零内存
    Node *tmp = m_root->next;
    while (tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }

    // 当前指针指向内存块头部
    m_cur = m_root;
    // 只有一块存储
    m_root->next = NULL;
}

/**
 * @brief 写入 size 长度的数据
 *
 * @param buf 内存缓存指针
 * @param size 数据大小
 */
void ByteArray::write(const void *buf, size_t size) {
    if (size == 0) {
        return;
    }
    // 扩容
    addCapacity(size);

    // 当前节点位置 node pos
    size_t npos = m_position % m_baseSize;
    // 当前节点的可用容量
    size_t ncap = m_cur->size - npos;
    // 指向 buf 当前位置 buffer pos
    size_t bpos = 0;

    // 写数据到节点 node
    while (size > 0) {
        if (ncap >= size) {
            // 当前节点容量足够 直接复制到当前节点
            memcpy(m_cur->ptr + npos, (const char *)buf + bpos, size);

            if (m_cur->size == (npos + size)) {
                // 下一个内存块
                m_cur = m_cur->next;
            }

            m_position += size;
            bpos += size;
            size = 0;
            // 写入完毕

        } else {
            // 当前节点不够 先填满该节点
            memcpy(m_cur->ptr + npos, (const char *)buf + bpos, ncap);

            m_position += ncap;
            bpos += ncap;
            size -= ncap;

            // 下一个内存块
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;

            // 继续循环写入
        }
    }

    if (m_position > m_size) {
        // 更新数据总大小
        m_size = m_position;
    }
}

/**
 * @brief 读取 size 长度的数据
 *
 * @param buf 结果缓存指针
 * @param size 数据大小
 */
void ByteArray::read(void *buf, size_t size) {
    if (size > getReadSize()) {
        // 大于可以读取的长度 抛出异常
        throw std::out_of_range("not enough len");
    }

    // 当前块号
    size_t npos = m_position % m_baseSize;
    // 当前块容量
    size_t ncap = m_cur->size - npos;
    // 结果缓存位置
    size_t bpos = 0;

    while (size > 0) {
        if (ncap >= size) {
            // 直接读取
            memcpy((char *)buf + bpos, m_cur->ptr + npos, size);

            if (m_cur->size == npos + size) {
                // 下一块存储
                m_cur = m_cur->next;
            }

            m_position += size;
            bpos += size;
            size = 0;
            // 读取完成

        } else {
            // 读完当前块
            memcpy((char *)buf + bpos, m_cur->ptr + npos, ncap);

            m_position += ncap;
            bpos += ncap;
            size -= ncap;

            // 下一块存储
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;

            // 继续循环读取
        }
    }
}

/**
 * @brief 从位置 position 开始读取 size 长度的数据 不会修改 m_position
 *
 * @param buf 结果缓存指针
 * @param size 数据大小
 * @param position 读取位置
 */
void ByteArray::read(void *buf, size_t size, size_t position) const {
    if (size > (m_size - position)) {
        // 大于可以读取的长度 抛出异常
        throw std::out_of_range("not enough len");
    }

    // 当前块号
    size_t npos = position % m_baseSize;
    // 当前块容量
    size_t ncap = m_cur->size - npos;
    // 结果缓存位置
    size_t bpos = 0;

    Node *cur = m_cur;
    while (size > 0) {
        if (ncap >= size) {
            // 直接读取
            memcpy((char *)buf + bpos, cur->ptr + npos, size);

            if (cur->size == npos + size) {
                // 下一块存储
                cur = cur->next;
            }

            position += size;
            bpos += size;
            size = 0;
            // 读取完成

        } else {
            // 读完当前块
            memcpy((char *)buf + bpos, cur->ptr + npos, ncap);

            position += ncap;
            bpos += ncap;
            size -= ncap;

            // 下一块存储
            cur = cur->next;
            ncap = cur->size;
            npos = 0;

            // 继续循环读取
        }
    }
}

/**
 * @brief 设置当前操作位置
 *
 * @param v
 */
void ByteArray::setPostion(size_t v) {
    // if (v > m_capacity)
    // {
    //     throw std::out_of_range("setPosition out of range");
    // }
    // m_position = v;
    // m_cur = m_root;

    if (v > m_capacity) {
        // 大于容量 抛出异常
        throw std::out_of_range("set_position out of range");
    }
    // 设置当前操作位置
    m_position = v;

    if (m_position > m_size) {
        // 更新当前数据总量
        m_size = m_position;
    }

    // 当前内存块指针指向头部
    m_cur = m_root;
    while (v > m_cur->size) {
        // 循环访问下一块
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if (v == m_cur->size) {
        // 当前内存块已经满了 下一个
        m_cur = m_cur->next;
    }
    // m_cur 已经指向当前操作的内存块了 即 m_position 所在
}

/// 文件操作
/**
 * @brief 写入到文件
 *
 * @param name 文件名
 * @return true
 * @return false
 */
bool ByteArray::writeToFile(const std::string &name) const {
    // 二进制方式打开文件
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if (!ofs) {
        // 打开失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "writeToFile name=" << name << " error, errno=" << errno
            << " errno-string=" << strerror(errno);
        return false;
    }

    // 可读取长度
    int64_t read_size = getReadSize();
    // 当前位置
    int64_t pos = m_position;
    // 当前内存块
    Node *cur = m_cur;

    while (read_size > 0) {
        // 内存块偏移
        int diff = pos % m_baseSize;
        // 读取长度
        int64_t len =
            (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;

        // 向文件写入
        ofs.write(cur->ptr + diff, len);

        // 下一内存块
        cur = cur->next;
        // 当前位置
        pos += len;
        // 还剩多少
        read_size -= len;
    }

    return true;
}

/**
 * @brief 从文件读取
 *
 * @param name 文件名
 * @return true
 * @return false
 */
bool ByteArray::readFromFile(const std::string &name) {
    // 二进制方式打开文件
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if (!ifs) {
        // 打开失败
        LJRSERVER_LOG_ERROR(g_logger)
            << "readFromFile name = " << name << " error, errno = " << errno
            << " errno-string = " << strerror(errno);
        return false;
    }

    // 智能指针自定义析构方法
    std::shared_ptr<char> buff(new char[m_baseSize], [](char *ptr) {
        // 清零内存
        delete[] ptr;
    });

    while (!ifs.eof()) {
        // 按内存块大小读取数据到缓存 buff.get() 获取内存裸指针
        ifs.read(buff.get(), m_baseSize);
        // 将缓存数据写入存储
        write(buff.get(), ifs.gcount());
    }

    return true;
}

/// 大小端字节序
/**
 * @brief 是否是小端字节序
 *
 * @return true 小端字节序
 * @return false 大端字节序
 */
bool ByteArray::isLittleEndian() const {
    return m_endian == LJRSERVER_LITTLE_ENDIAN;
}

/**
 * @brief 设置是否为小端字节序
 *
 * @param val
 */
void ByteArray::setLittleEndian(bool val) {
    if (val) {
        m_endian = LJRSERVER_LITTLE_ENDIAN;
    } else {
        m_endian = LJRSERVER_BIG_ENDIAN;
    }
}

/// 输出字符串
/**
 * @brief  将 ByteArray 里面的数据 [m_position, m_size) 转成 std::string
 *
 * @return std::string
 */
std::string ByteArray::toString() const {
    // 输出缓存
    std::string str;
    str.resize(getReadSize());
    if (str.empty()) {
        return str;
    }

    // 读取所有可以读取的数据
    read(&str[0], str.size(), m_position);
    return str;
}

/**
 * @brief 将 ByteArray 里面的数据 [m_position, m_size) 转成
 * 16 进制的 std::string (格式: FF FF FF)
 *
 * @return std::string
 */
std::string ByteArray::toHexString() const {
    // 调用 ByteArray::toString();
    std::string str = toString();
    // 字符串流
    std::stringstream ss;

    for (size_t i = 0; i < str.size(); ++i) {
        // 4byte 换行
        if (i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        // 格式: FF FF FF
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    // 返回 std::string
    return ss.str();
}

/// 获取读写缓存
/**
 * @brief 获取可读取的缓存 保存至 iovec 数组
 *
 * 只获取内容，不修改 position
 *
 * @param buffers 保存可读取数据的 iovec 数组
 * @param len 读取数据的长度
 * @return uint64_t 返回实际读到数据的长度
 */
uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers,
                                   uint64_t len) const {
    // 读取长度
    len = len > getReadSize() ? getReadSize() : len;
    if (len == 0) {
        return 0;
    }
    // 返回实际数据的长度
    uint64_t size = len;

    // 当前内存块的偏移
    size_t npos = m_position % m_baseSize;
    // 当前内存块可读取的容量
    size_t ncap = m_cur->size - npos;

    // 输出数据
    struct iovec iov;

    Node *cur = m_cur;
    while (len > 0) {
        if (ncap >= len) {
            // 直接读取
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;

            // 下一内存块
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }

        // 读取到的数据加入结果缓存
        buffers.push_back(iov);
    }

    // 返回读到的实际数据的长度
    return size;
}

/**
 * @brief 从 position 位置开始 获取可读取的缓存 保存至 iovec 数组
 *
 * 只获取内容，不修改 position
 *
 * @param buffers 保存可读取数据的 iovec 数组
 * @param len 读取数据的长度
 * @param position 读取数据的位置
 * @return uint64_t 返回实际读到数据的长度
 */
uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                                   uint64_t position) const {
    // 读取长度
    len = len > getReadSize() ? getReadSize() : len;
    if (len == 0) {
        return 0;
    }
    // 返回实际数据的长度
    uint64_t size = len;

    // 内存块的偏移
    size_t npos = position % m_baseSize;
    // 存储块数
    size_t count = position / m_baseSize;

    // 找到 position 所在的存储块
    Node *cur = m_root;
    while (count > 0) {
        cur = cur->next;
        --count;
    }

    // 当前内存块可读取的容量
    size_t ncap = cur->size - npos;

    // 输出数据
    struct iovec iov;
    // Node *tmp = m_cur;
    while (len > 0) {
        if (ncap >= len) {
            // 直接读取
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;

            // 下一内存块
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }

        // 读取到的数据加入结果缓存
        buffers.push_back(iov);
    }

    // 返回读到的实际数据的长度
    return size;
}

/**
 * @brief 获取可写入的缓存 保存至 iovec 数组
 *
 * 可能会增加容量 但不修改 position
 *
 * @param buffers 保存可写入的内存的 iovec 数组
 * @param len 写入的长度
 * @return uint64_t 返回实际的长度
 */
uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len) {
    if (len == 0)
        return 0;

    // 是否扩容
    addCapacity(len);
    // 返回获取的长度
    size_t size = len;

    // 当前内存块偏移
    size_t npos = m_position % m_baseSize;
    // 当前内存块可读取的容量
    size_t ncap = m_cur->size - npos;

    // 输出数据
    struct iovec iov;

    Node *cur = m_cur;
    while (len > 0) {
        if (ncap >= size) {
            // 直接读取
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;

            // 下一内存块
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }

        // 读取到的数据加入结果缓存
        buffers.push_back(iov);
    }

    // 返回获取的内存块长度
    return size;
}

/// private
/**
 * @brief 扩容 private
 *
 * @param size 容量大小
 */
void ByteArray::addCapacity(size_t size) {
    if (size == 0) {
        return;
    }
    // 获取可用容量
    size_t old_cap = getCapacity();
    if (old_cap >= size) {
        // 不用扩容
        return;
    }

    // 需要扩大的容量大小
    size = size - old_cap;
    // 新增 count 个内存块
    size_t count =
        (size / m_baseSize) + (((size % m_baseSize) > old_cap) ? 1 : 0);

    // 找到节点尾部
    Node *tmp = m_root;
    while (tmp->next) {
        tmp = tmp->next;
    }

    Node *first = NULL;
    for (size_t i = 0; i < count; ++i) {
        // 新建节点插入尾部
        tmp->next = new Node(m_baseSize);
        if (first == NULL) {
            // 指向首个新增节点
            first = tmp->next;
        }
        // 更新尾部节点
        tmp = tmp->next;
        // 容量 ++
        m_capacity += m_baseSize;
    }

    if (old_cap == 0) {
        // 首次扩容
        m_cur = first;
    }
}

}  // namespace ljrserver
