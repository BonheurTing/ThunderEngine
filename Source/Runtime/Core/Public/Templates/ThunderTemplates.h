#pragma once

namespace Thunder
{
    //禁止拷贝类，继承此类则不可拷贝
    class Noncopyable
    {
    protected:
        // 确保不能直接构造类
        Noncopyable() = default;
        // 类不应该以多态方式使用
        ~Noncopyable() = default;
    private:
        Noncopyable(const Noncopyable&);
        Noncopyable& operator=(const Noncopyable&);
    };
}
