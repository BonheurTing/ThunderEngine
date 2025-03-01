#pragma once
#include "Container.h"
#include "NameHandle.h"
#include "Platform.h"
#include "Concurrent/TheadPool.h"

namespace Thunder
{

//线程基类，本身不包含线程，可以根据平台API创建线程；是一个接口，用于管理可运行线程的生命周期
class IThread
{
public:
    // 工厂方法，用于创建具有指定堆栈大小和线程优先级的线程
    static IThread* Create(class ThreadProxy* InProxy, uint32 InStackSize = 0, const String& InThreadName = "");

    // 告知线程暂停执行或恢复执行
    virtual void Suspend( bool bShouldPause = true ) = 0;

    // 告知线程退出，并可选择等待线程退出。强制销毁线程可能会导致资源泄漏和死锁
    virtual bool Kill( bool bShouldWait = true ) = 0;

    // 阻塞调用线程，直到该线程完成其工作
    virtual void WaitForCompletion() = 0;

    uint32 GetThreadID() const { return ThreadID; }
    NameHandle GetName() const { return DebugName; }

    IThread() {}

    virtual ~IThread() {}

protected:
    virtual bool CreateInternal(ThreadProxy* InProxy, uint32 InStackSize = 0, const String& InThreadName = "") = 0;

    ThreadProxy* Proxy = nullptr;

    // 线程被创建时设置的ID
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
