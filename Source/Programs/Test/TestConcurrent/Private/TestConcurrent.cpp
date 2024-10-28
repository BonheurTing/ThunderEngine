#pragma optimize("", off)
#include "Misc/Lock.h"

using namespace Thunder;

int main()
{
    //test lock
    SpinLock GTestSpintLock;
    SharedLock GTestSharedLock;
    RecursiveLock GTestRecursiveLock;
    {
        auto guard = GTestSpintLock.Guard();
        // Do something
    }
    {
        TLockGuard<SpinLock> guard(GTestSpintLock);
        // Do something
    }
    {
        auto guard = GTestSharedLock.Read();
        // ...
    }
    {
        auto guard = GTestSharedLock.Write();
        // ...
    }
    {
        auto guard = GTestRecursiveLock.Guard();
        // ...
    }
    
    //system("pause");
    return 0;
}
#pragma optimize("", on)

 