
#ifndef __LJRSERVER_BYTEARRAY_H__
#define __LJRSERVER_BYTEARRAY_H__

#include <memory>
#include <string>
#include <vector>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace ljrserver {

/**
 * @brief Class 二进制数组
 *
 * 提供基础类型的序列化 反序列化功能
 *
 */
class ByteArray {
public:
    // 智能指针
    typedef std::shared_ptr<ByteArray> ptr;

    /**
     * @brief ByteArray 的存储节点 内存块
     *
     * 结构体、类 初始化时，赋值顺序必须和定义顺序相同
     * （错误）结构体中，指向自身的指针 Node *next; 只能放在最后初始化
     *
     */
    struct Node {
        /**
         * @brief 数据节点构造函数 重载
         *
         * @param s
         */
        Node(size_t s);

        /**
         * @brief 数据节点构造函数 默认
         *
         * 默认内存块大小为零
         */
        Node();

        /**
         * @brief 数据节点析构函数
         *
         */
        ~Node();

        /// 默认 public:
        /// 内存块地址指针
        char *ptr;

        /// 内存块大小
        size_t size;

        /// 下一个内存块地址
        Node *next;
    };

    /**
     * @brief 构造函数
     *
     * @param base_size 存储块大小 [= 4kb]
     */
    ByteArray(size_t base_size = 4096);

    /**
     * @brief 析构函数
     *
     */
    ~ByteArray();

public:  /// Write 写入
    /// 固定长度

    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);

    /// 变长
    // void writeInt8(const int8_t &value);
    // void writeUint8(const uint8_t &value);
    // void writeInt16(const int16_t &value);
    // void writeUint16(const uint16_t &value);

    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    /// 浮点数

    void writeFloat(float value);
    void writeDouble(double value);

    /// 字符串

    void writeStringF16(const std::string &value);
    void writeStringF32(const std::string &value);
    void writeStringF64(const std::string &value);
    void writeStringVint(const std::string &value);
    void writeStringWithoutLength(const std::string &value);

public:  /// Read 读取
    /// 固定长度

    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();

    /// 变长

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    /// 浮点数

    float readFloat();
    double readDouble();

    /// 字符串

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

public:  /// 内部操作
    /**
     * @brief 清空数据
     *
     */
    void clear();

    /**
     * @brief 写入 size 长度的数据
     *
     * @param buf 内存缓存指针
     * @param size 数据大小
     */
    void write(const void *buf, size_t size);

    /**
     * @brief 读取 size 长度的数据
     *
     * @param buf 结果缓存指针
     * @param size 数据大小
     */
    void read(void *buf, size_t size);

    /**
     * @brief 从位置 position 开始读取 size 长度的数据
     *
     * @param buf 结果缓存指针
     * @param size 数据大小
     * @param position 读取位置
     */
    void read(void *buf, size_t size, size_t position) const;

    /**
     * @brief 设置当前操作位置
     *
     * @param v
     */
    void setPostion(size_t v);

    /**
     * @brief 获取当前操作位置
     *
     * @return size_t
     */
    size_t getPosition() const { return m_position; }

    /**
     * @brief 获取存储块基础大小
     *
     * @return size_t
     */
    size_t getBaseSize() const { return m_baseSize; }

    /**
     * @brief 获取可读长度
     *
     * @return size_t
     */
    size_t getReadSize() const { return m_size - m_position; }

    /**
     * @brief 获取当前数据量大小
     *
     * @return size_t
     */
    size_t getSize() const { return m_size; }

public:  /// 文件操作
    /**
     * @brief 写入到文件
     *
     * @param name 文件名
     * @return true
     * @return false
     */
    bool writeToFile(const std::string &name) const;

    /**
     * @brief 从文件读取
     *
     * @param name 文件名
     * @return true
     * @return false
     */
    bool readFromFile(const std::string &name);

public:  /// 大小端字节序
    /**
     * @brief 是否是小端字节序
     *
     * @return true 小端字节序
     * @return false 大端字节序
     */
    bool isLittleEndian() const;

    /**
     * @brief 设置是否为小端字节序
     *
     * @param val
     */
    void setLittleEndian(bool val);

public:  /// 输出字符串
    /**
     * @brief  将 ByteArray 里面的数据 [m_position, m_size] 转成 std::string
     *
     * @return std::string
     */
    std::string toString() const;

    /**
     * @brief 将 ByteArray 里面的数据 [m_position, m_size) 转成
     * 16 进制的 std::string (格式: FF FF FF)
     *
     * @return std::string
     */
    std::string toHexString() const;

public:  /// 获取读写缓存
    /**
     * @brief 获取可读取的缓存 保存至 iovec 数组
     *
     * 只获取内容，不修改 position
     *
     * @param buffers 保存可读取数据的 iovec 数组
     * @param len 读取数据的长度
     * @return uint64_t 返回实际读到数据的长度
     */
    uint64_t getReadBuffers(std::vector<iovec> &buffers,
                            uint64_t len = ~0ull) const;

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
    uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                            uint64_t position) const;

    /**
     * @brief 获取可写入的缓存 保存至 iovec 数组
     *
     * 可能会增加容量 但不修改 position
     *
     * @param buffers 保存可写入的内存的 iovec 数组
     * @param len 写入的长度
     * @return uint64_t 返回实际的长度
     */
    uint64_t getWriteBuffers(std::vector<iovec> &buffers, uint64_t len);

private:
    /**
     * @brief 获取可用容量 private
     *
     * @return size_t 容量大小
     */
    size_t getCapacity() const { return m_capacity - m_position; }

    /**
     * @brief 扩容 private
     *
     * @param size 容量大小
     */
    void addCapacity(size_t size);

private:
    // 内存块大小
    size_t m_baseSize;

    // 当前操作位置
    size_t m_position;

    // 总容量
    size_t m_capacity;

    // 当前数据量大小
    size_t m_size;

    // 字节序 默认大端
    int m_endian;

    // 第一个内存块指针
    Node *m_root;

    // 当前操作的内存块指针
    Node *m_cur;
};

}  // namespace ljrserver

#endif  //__LJRSERVER_BYTEARRAY_H__