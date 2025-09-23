#pragma once

namespace Thunder
{
    class Noncopyable
    {
    protected:
        // Make sure not to directly construct classes
        Noncopyable() = default;
        // Classes should not be used in a polymorphic manner.
        ~Noncopyable() = default;
    private:
        Noncopyable(const Noncopyable&);
        Noncopyable& operator=(const Noncopyable&);
    };
}
