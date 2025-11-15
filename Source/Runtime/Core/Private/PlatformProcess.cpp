#include "PlatformProcess.h"
#include <chrono>
#include "intrin.h"
#include "Misc/LazySingleton.h"
#include "Windows/WindowsThread.h"
#include "Platform.h"

namespace Thunder
{
    IEvent* FPlatformProcess::CreateSyncEvent(bool bIsManualReset)
    {
        IEvent* Event = nullptr;	
        if (SupportsMultithreading())
        {
            Event = new EventWindows();
        }

        // If the internal create fails, delete the instance and return NULL
        if (!Event->Create(bIsManualReset))
        {
            delete Event;
            Event = nullptr;
        }
        return Event;
    }

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

    int32 FPlatformProcess::NumberOfCores()
    {
#if THUNDER_WINDOWS
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return static_cast<int32>(sysInfo.dwNumberOfProcessors);
#elif THUNDER_POSIX
        long numCores = sysconf(_SC_NPROCESSORS_ONLN);
        return static_cast<int32>(numCores > 0 ? numCores : 1);
#endif
        return 0;
    }

    DWORD CountSetBits(ULONG_PTR bitMask) {
        DWORD count = 0;
        while (bitMask) {
            count += bitMask & 1;
            bitMask >>= 1;
        }
        return count;
    }

    int32 FPlatformProcess::NumberOfLogicalProcessors()
    {
#if THUNDER_WINDOWS
        DWORD logicalProcessorCount = 0;
        DWORD physicalCoreCount = 0;
        // 获取缓冲区大小
        DWORD bufferSize = 0;
        GetLogicalProcessorInformation(nullptr, &bufferSize);

        // 分配缓冲区
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(buffer.data(), &bufferSize)) {
            for (const auto& info : buffer) {
                if (info.Relationship == RelationProcessorCore) {
                    // 每个物理核可能支持多个逻辑处理器（超线程）
                    physicalCoreCount++;
                    logicalProcessorCount += CountSetBits(info.ProcessorMask);
                }
            }
        } else {
            std::cerr << "获取处理器信息失败！" << std::endl;
            return 8;
        }
        return static_cast<int32>(logicalProcessorCount);
#elif THUNDER_POSIX
        // POSIX系统使用sysconf获取逻辑处理器数量
        long logicalProcessorCount = sysconf(_SC_NPROCESSORS_ONLN);
        return static_cast<int32>(logicalProcessorCount > 0 ? logicalProcessorCount : 1);
#endif
        return 0;
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

    void FPlatformProcess::Sleep( float seconds )
    {
        uint32 milliseconds = (uint32)(seconds * 1000.0);
        if (milliseconds == 0)
        {
            ::SwitchToThread();
        }
        else
        {
            ::Sleep(milliseconds);
        }
    }

    void FPlatformProcess::BusyWaiting(int32 us)
    {
        auto start = std::chrono::high_resolution_clock::now();
        volatile int x = 0; // 使用 volatile 防止优化
        while (true) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
            if (elapsed >= us) {
                break;
            }
            ++x; // 忙等待
            FPlatformProcess::CoreYield();
        }
    }
}
