#include "HAL/Thread.h"
#include "Assertion.h"
#include "PlatformProcess.h"

namespace Thunder
{

IThread* IThread::Create(class ThreadProxy* InRunnable, const String& ThreadName, uint32 InStackSize, EThreadPriority InThreadPri)
{
    bool bCreateRealThread = FPlatformProcess::SupportsMultithreading();

    IThread* NewThread = nullptr;

    if (bCreateRealThread)
    {
        TAssert(InRunnable);
        // Create a new thread object
        NewThread = FPlatformProcess::CreateRunnableThread();
    }

    if (NewThread)
    {
        // Call the thread's create method
        bool bIsValid = NewThread->CreateInternal(InRunnable, ThreadName, InStackSize, InThreadPri);

        if( bIsValid )
        {
            TAssert(NewThread->Runnable);
        }
        else
        {
            // We failed to start the thread correctly so clean up
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
