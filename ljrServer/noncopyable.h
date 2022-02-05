
#ifndef __LJRSERVER_NONCOPYABLE_H__
#define __LJRSERVER_NONCOPYABLE_H__

namespace ljrserver {

/**
 * @brief 不允许复制
 *
 * 只需继承该类即可禁用复制构造
 */
class Noncopyable {
public:
    /**
     * @brief 默认构造函数
     *
     */
    Noncopyable() = default;

    /**
     * @brief 默认析构函数
     *
     */
    ~Noncopyable() = default;

    /**
     * @brief 禁用 () 复制构造函数
     *
     */
    Noncopyable(const Noncopyable &) = delete;

    /**
     * @brief 禁用 = 复制赋值函数
     *
     * @return Noncopyable&
     */
    Noncopyable &operator=(const Noncopyable &) = delete;
};

}  // namespace ljrserver

#endif  //__LJRSERVER_NONCOPYABLE_H__