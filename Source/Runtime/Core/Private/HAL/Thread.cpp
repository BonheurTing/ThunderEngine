#include "HAL/Thread.h"
#include "Assertion.h"
#include "PlatformProcess.h"

namespace Thunder
{

IThread* IThread::Create(class ThreadProxy* InProxy, uint32 InStackSize, const String& InThreadName)
{
    const bool bCreateRealThread = FPlatformProcess::SupportsMultithreading();

    IThread* NewThread = nullptr;

    if (bCreateRealThread)
    {
        TAssert(InProxy);
        NewThread = FPlatformProcess::CreateRunnableThread(); //直接创建threadWindows
    }

    if (NewThread)
    {
        if (NewThread->CreateInternal(InProxy, InStackSize, InThreadName))
        {
            TAssert(NewThread->Proxy);
        }
        else
        {
            delete NewThread;
            NewThread = nullptr;
        }
    }

    return NewThread;
}


ThreadManager& ThreadManager::Get()
{
    static ThreadManager Singleton;
    return Singleton;
}
    
}
