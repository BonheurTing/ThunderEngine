#pragma once
#include "Container.h"
#include "NameHandle.h"
#include "Platform.h"
#include "Concurrent/TheadPool.h"

namespace Thunder
{

// Interface for threads, used for thread lifetime managing.
class IThread
{
public:
    static IThread* Create(class ThreadProxy* InProxy, uint32 InStackSize = 0, const String& InThreadName = "");

    virtual void Suspend( bool bShouldPause = true ) = 0;

    virtual bool Kill( bool bShouldWait = true ) = 0;

    virtual void WaitForCompletion() = 0;

    uint32 GetThreadID() const { return ThreadID; }
    NameHandle GetName() const { return DebugName; }

    IThread() {}

    virtual ~IThread() {}

protected:
    virtual bool CreateInternal(ThreadProxy* InProxy, uint32 InStackSize = 0, const String& InThreadName = "") = 0;

    ThreadProxy* Proxy = nullptr;

    uint32 ThreadID = 0;

    NameHandle DebugName;
};

class ThreadManager
{
public:
    ~ThreadManager();
    CORE_API void AddThread(ThreadProxy* Thread) { TAssert(Thread); Threads.push_back(Thread); }
    CORE_API void AddThreadPool(class ThreadPoolBase* InThreadPool) { ThreadPools.push_back(InThreadPool); }
    static CORE_API ThreadManager& Get();
private:
    TArray<ThreadProxy*> Threads;
    TArray<ThreadPoolBase*> ThreadPools;
};
}
