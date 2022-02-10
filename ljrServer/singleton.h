

#ifndef __LJRSERVER_SINGLETON_H__
#define __LJRSERVER_SINGLETON_H__

// 智能指针
#include <memory>

namespace ljrserver {

/**
 * @brief 模版类实现的单例模式
 *
 * @tparam T
 * @tparam X
 * @tparam N
 */
template <class T, class X = void, int N = 0>
class Singleton {
public:
    /**
     * @brief 获取唯一实例
     * 
     * static 函数能确保初始化顺序
     *
     * @return T*
     */
    static T *GetInstance() {
        // 全局的唯一实例
        static T v;
        return &v;
    }
};

template <class T, class X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}  // namespace ljrserver

#endif