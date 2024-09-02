#include "Windows/WindowsThread.h"
#include "Misc/TheadPool.h"
#include "PlatformProcess.h"

namespace Thunder
{
bool EventWindows::Wait( uint32 WaitTime, const bool bIgnoreThreadIdleStats)
{
    TAssert(Event);
    return (WaitForSingleObject( Event, WaitTime ) == WAIT_OBJECT_0);
}

uint32 ThreadWindows::GuardedRun()
{
    uint32 ExitCode = 0;

   //todo

    return ExitCode;
}

uint32 ThreadWindows::Run()
{
    uint32 ExitCode = 1;
    TAssert(Runnable);

    if (Runnable->Init() == true)
    {
        ThreadInitSyncEvent->Trigger();

        // Setup TLS for this thread, used by FTlsAutoCleanup objects.
        SetTls();

        ExitCode = Runnable->Run();

        // Allow any allocated resources to be cleaned up
        Runnable->Exit();

        FreeTls();
    }
    else
    {
        // Initialization has failed, release the sync event
        ThreadInitSyncEvent->Trigger();
    }

    return ExitCode;
}

bool ThreadWindows::Kill(bool bShouldWait)
{
    TAssert(Thread && "Did you forget to call Create()?");
    bool bDidExitOK = true;

    // Let the runnable have a chance to stop without brute force killing
    if (Runnable)
    {
        Runnable->Stop();
    }

    if (bShouldWait == true)
    {
        // Wait indefinitely for the thread to finish.  IMPORTANT:  It's not safe to just go and
        // kill the thread with TerminateThread() as it could have a mutex lock that's shared
        // with a thread that's continuing to run, which would cause that other thread to
        // dead-lock.  
        //
        // This can manifest itself in code as simple as the synchronization
        // object that is used by our logging output classes

        WaitForSingleObject(Thread, INFINITE);
    }

    CloseHandle(Thread);
    Thread = NULL;

    return bDidExitOK;
}
    
bool ThreadWindows::CreateInternal(ThreadProxy* InRunnable, const String& InThreadName, uint32 InStackSize, EThreadPriority InThreadPri)
{
    static bool bOnce = false;
    if (!bOnce)
    {
        bOnce = true;
        ::SetThreadPriority(::GetCurrentThread(), TranslateThreadPriority(EThreadPriority::Normal));
    }

    TAssert(InRunnable);
    Runnable = InRunnable;

    // Create a sync event to guarantee the Init() function is called first
    ThreadInitSyncEvent = FPlatformProcess::GetSyncEventFromPool(true);

    //ThreadName = InThreadName ? InThreadName : TEXT("Unnamed UE");
    ThreadPriority = InThreadPri;
    
    // Create the thread as suspended, so we can ensure ThreadId is initialized and the thread manager knows about the thread before it runs.
    Thread = CreateThread(NULL, InStackSize, _ThreadProc, this, STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED, (::DWORD*)&ThreadID);


    // If it fails, clear all the vars
    if (Thread == NULL)
    {
        Runnable = nullptr;
    }
    else
    {
        ResumeThread(Thread);

        // Let the thread start up
        ThreadInitSyncEvent->Wait(INFINITE);

        ThreadPriority = EThreadPriority::Normal; // Set back to default in case any SetThreadPrio() impls compare against current value to reduce syscalls
        SetThreadPriority(InThreadPri);
    }

    // Cleanup the sync event
    FPlatformProcess::ReturnSyncEventToPool(ThreadInitSyncEvent);
    ThreadInitSyncEvent = nullptr;
    return Thread != NULL;
}
    
}
