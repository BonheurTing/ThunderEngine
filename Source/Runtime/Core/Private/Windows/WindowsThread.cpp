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
    if (bShouldWait)
    {
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

    Thread = CreateThread(nullptr, InStackSize, ThreadProc, this,
        STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED, reinterpret_cast<::DWORD*>(&ThreadID));

    if (Thread == nullptr)
    {
        Proxy = nullptr;
    }
    else
    {
        ResumeThread(Thread);
	    ThreadManager::Get().AddThread(Proxy);
    }

    return Thread != nullptr;
}
    
}
