#include "PlatformProcess.h"
#include "intrin.h"
#include "Misc/LazySingleton.h"
#include "Windows/WindowsThread.h"

namespace Thunder
{
    IEvent* FPlatformProcess::GetSyncEventFromPool(bool bIsManualReset)
    {
        return bIsManualReset
            ? TLazySingleton<TEventPool<EEventMode::ManualReset>>::Get().GetEventFromPool()
            : TLazySingleton<TEventPool<EEventMode::AutoReset>>::Get().GetEventFromPool();
    }

    void FPlatformProcess::ReturnSyncEventToPool(IEvent* Event)
    {
        if( !Event )
        {
            return;
        }

        if (Event->IsManualReset())
        {
            TLazySingleton<TEventPool<EEventMode::ManualReset>>::Get().ReturnToPool(Event);
        }
        else
        {
            TLazySingleton<TEventPool<EEventMode::AutoReset>>::Get().ReturnToPool(Event);
        }
    }
    
    IThread* FPlatformProcess::CreateRunnableThread()
    {
        return new ThreadWindows(); //FWindowsPlatformProcess::
    }

    void FPlatformProcess::CoreYield()
    {
#if PLATFORM_CPU_X86_FAMILY
        _mm_pause();
#elif PLATFORM_CPU_ARM_FAMILY
#	if !defined(__clang__)
        __yield(); // MSVC
    #	else
    __builtin_arm_yield();
#	endif
#else
#	error Unsupported architecture!
#endif
    }

    void FPlatformProcess::Sleep( float Seconds )
    {
        uint32 Milliseconds = (uint32)(Seconds * 1000.0);
        if (Milliseconds == 0)
        {
            ::SwitchToThread();
        }
        else
        {
            ::Sleep(Milliseconds);
        }
    }
}
