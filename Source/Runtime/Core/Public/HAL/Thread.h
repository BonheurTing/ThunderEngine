#pragma once
#include "Container.h"
#include "Platform.h"
#include "HAL/Event.h"

namespace Thunder
{

class ThreadProxy;
enum class EThreadPriority : uint8
{
    High,
    Normal,
    Low,
    Num
};

//FRunnableThread 线程基类，本身不包含线程，可以根据平台API创建线程；是一个接口，用于管理可运行线程的生命周期
class CORE_API IThread
{
    /** Index of TLS slot for FRunnableThread pointer. */
    static uint32 RunnableTlsSlot;

public:

    /** 获取一个新的TLS（线程本地存储）槽，用于存储可运行线程的指针 */
    static uint32 GetTlsSlot() { return 0; } //todo

    /** 工厂方法，用于创建具有指定堆栈大小和线程优先级的线程 */
    static IThread* Create(ThreadProxy* InRunnable, const String& ThreadName, uint32 InStackSize = 0, EThreadPriority InThreadPri = EThreadPriority::Normal);

    /** 更改当前运行线程的线程优先级 */
    virtual void SetThreadPriority( EThreadPriority NewPriority ) {} //todo

    /** 
     * 更改当前运行线程的线程亲和性
     * 线程亲和性（Thread Affinity）是指将线程绑定到特定的处理器核心或处理器组的能力。通过设置线程亲和性，可以控制线程在哪些处理器核心上运行，以实现更好的性能和资源管理
     * 在多核系统中，每个处理器核心都有自己的缓存和执行单元。通过将线程绑定到特定的处理器核心，可以减少线程在不同核心之间切换的开销，提高缓存利用率，并避免竞争条件和锁竞争
     * 线程亲和性通常通过设置一个位掩码来表示，每个位对应一个处理器核心或处理器组。将线程亲和性掩码设置为特定的值，可以将线程限制在指定的核心或组上运行
     */
    virtual bool SetThreadAffinity() { return false; } //todo

    /** 告知线程暂停执行或恢复执行 */
    virtual void Suspend( bool bShouldPause = true ) = 0;

    /** 告知线程退出，并可选择等待线程退出。强制销毁线程可能会导致资源泄漏和死锁 */
    virtual bool Kill( bool bShouldWait = true ) = 0;

    /** 阻塞调用线程，直到该线程完成其工作. */
    virtual void WaitForCompletion() = 0;

    const uint32 GetThreadID() const { return ThreadID; }

    EThreadPriority GetThreadPriority() const { return ThreadPriority; }

    IThread() {}

    virtual ~IThread() {}

protected:
    virtual bool CreateInternal(ThreadProxy* InRunnable, const String& InThreadName,uint32 InStackSize = 0, EThreadPriority InThreadPri = EThreadPriority::Normal) = 0;
    void SetTls() {} //todo
    void FreeTls() {} //todo
    static IThread* GetRunnableThread()
    {
        return nullptr; //todo
    }

    /** The runnable object to execute on this thread. */
    ThreadProxy* Runnable;

    /** Sync event to make sure that Init() has been completed before allowing the main thread to continue. */
    IEvent* ThreadInitSyncEvent;

    /** The priority to run the thread at. */
    EThreadPriority ThreadPriority;

    /** ID set during thread creation. */
    uint32 ThreadID;
};

class ThreadManager
{
public:

    CORE_API void AddThread(uint32 ThreadId, IThread* Thread) {};
    static CORE_API ThreadManager& Get();
};
}
