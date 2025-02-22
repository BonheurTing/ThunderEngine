#include "Windows/WindowsThread.h"
#include "Concurrent/TheadPool.h"
#include "PlatformProcess.h"

namespace Thunder
{
bool EventWindows::Wait( uint32 WaitTime, const bool bIgnoreThreadIdleStats)
{
    TAssert(Event);
    return (WaitForSingleObject( Event, WaitTime ) == WAIT_OBJECT_0);
}

uint32 ThreadWindows::Run()
{
    TAssert(Proxy);
    return Proxy->Run();
}

bool ThreadWindows::Kill(bool bShouldWait)
{
    TAssert(Thread && "Did you forget to call Create()?");
    if (Proxy)
    {
        Proxy->Stop();
    }
    if (bShouldWait)
    {
        // 等待线程完成。不能直接用TerminateThread()杀死线程，因为它可能和正常运行的线程共享互斥锁，导致别人死锁
        WaitForSingleObject(Thread, INFINITE);
    }

    CloseHandle(Thread);
    Thread = nullptr;

    return true;
}
    
bool ThreadWindows::CreateInternal(ThreadProxy* InProxy, uint32 InStackSize, const String& InThreadName)
{
    TAssert(InProxy);
    Proxy = InProxy;

    DebugName = InThreadName;
    
    /** 创建线程的真正入口：在程序进程的虚拟地址空间创建线程
    * 参数1，lpThreadAttributes：确定返回的句柄是否可由子进程继承
    * ThreadProc: 回调入口
    * flag: 创建时挂起，保证threadId被初始化，ThreadManager可管理
    * reinterpret_cast: 不改变位上的值，只重新解释，适合cast指针或者引用
    **/
    Thread = CreateThread(nullptr, InStackSize, ThreadProc, this,
        STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED, reinterpret_cast<::DWORD*>(&ThreadID));

    if (Thread == nullptr)
    {
        Proxy = nullptr;
    }
    else
    {
        /** 减少挂起计数，计数为0时，线程恢复执行
         * 函数成功，返回线程之前的挂起计数；函数失败返回-1，可用GetLastError获取更多信息
         * CreateThread时挂起了，所以此处resume
         **/
        ResumeThread(Thread);
    }

    return Thread != nullptr;
}
    
}
