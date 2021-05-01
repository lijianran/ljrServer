
#ifndef __LJRSERVER_NONCOPYABLE_H__
#define __LJRSERVER_NONCOPYABLE_H__

namespace ljrserver
{

    class Noncopyable
    {
    public:
        Noncopyable() = default;
        ~Noncopyable() = default;
        Noncopyable(const Noncopyable &) = delete;
        Noncopyable &operator=(const Noncopyable &) = delete;
    };

} // namespace ljrserver

#endif //__LJRSERVER_NONCOPYABLE_H__