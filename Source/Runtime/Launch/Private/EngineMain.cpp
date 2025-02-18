#pragma optimize("", off)
#include "EngineMain.h"
#include "CoreModule.h"
#include "D3D12RHIModule.h"
#include "D3D11RHIModule.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"
#include "Memory/MallocMinmalloc.h"
#include "Misc/TheadPool.h"
#include "Module/ModuleManager.h"
#include "tracy/Tracy.hpp"

namespace Thunder
{
    ThreadProxy* GGameThread {};
    ThreadProxy* GRenderThread {};
    ThreadProxy* GRHIThread {};
    std::atomic<uint32> GFrameNumberGameThread {0};
    std::atomic<uint32> GFrameNumberRenderThread {0};
    std::atomic<uint32> GFrameNumberRHIThread {0};
    SimpleLock* GGameRenderLock {};
    SimpleLock* GRenderRHILock {};
    
    bool GIsRequestingExit {false};
    SimpleLock* GThunderEngineLock {};

    ThreadPoolBase* SyncWorkers {};
    ThreadPoolBase* AsyncWorkers {};

    bool IsEngineExitRequested()
    {
        return GIsRequestingExit;
    }

    void BusyWaiting(int32 i)
    {
        while(i-- > 0)
        {
        }
    }

    void PhysicsTask::DoWorkInner()
    {
        ZoneScoped;
        BusyWaiting(100); //debug tracy
        LOG("Execute physical calculation(data: %d) with thread: %lu", Data, __threadid());
    }

    void CullTask::DoWorkInner()
    {
        ZoneScoped;
        BusyWaiting(100); //debug tracy
        LOG("Execute clipping calculation(data: %d) with thread: %lu", Data, __threadid());
    }

    void TickTask::DoWorkInner()
    {
        ZoneScoped;
        BusyWaiting(100); //debug tracy
        LOG("Execute tick calculation(data: %d) with thread: %lu", Data, __threadid());
        if (GFrameNumberGameThread.fetch_add(1, std::memory_order_acq_rel) >= 100000)
        {
            GIsRequestingExit = true;
        }
    }

    void GameThread::EngineLoop()
    {
        while( !IsEngineExitRequested() )
        {
            GameMain();

            // push render command
            const auto testRenderThreadTask = new TTask<RenderingThread>(0);
            GRenderThread->PushTask(testRenderThreadTask);
        }

        LOG("End GameThread Execute");
        GThunderEngineLock->cv.notify_all();
    }

    void GameThread::GameMain()
    {
        FrameMark;

        ZoneScoped;
        BusyWaiting(1000); //debug tracy
        
        TaskGraph->Reset(); //等待上一帧执行完

        if (GFrameNumberGameThread.load() - GFrameNumberRenderThread.load() > 1)
        {
            std::unique_lock<std::mutex> lock(GGameRenderLock->mtx);
            GGameRenderLock->cv.wait(lock, []{ return GFrameNumberGameThread.load() - GFrameNumberRenderThread.load() <= 1; });
        }

        const int32 FrameNum = static_cast<int32>(GFrameNumberGameThread.load(std::memory_order_acquire));
        // game thread
        LOG("Execute game thread in frame: %d with thread: %lu", FrameNum, __threadid());

        
        // physics
        auto* TaskPhysics = new PhysicsTask(FrameNum, "PhysicsTask");
        // cull
        auto* TaskCull = new CullTask(FrameNum, "CullTask");
        // tick
        auto* TaskTick = new TickTask(FrameNum, "TickTask");

        // Task Graph
        TaskGraph->PushTask(TaskPhysics);
        TaskGraph->PushTask(TaskCull, {TaskPhysics});
        TaskGraph->PushTask(TaskTick, {TaskCull});

        TaskGraph->Submit();
    }

    void GameThread::Init()
    {
        auto* WorkerThreadPool = new ThreadPoolBase();
        WorkerThreadPool->Create(8, 96 * 1024);
        TaskGraph = new TaskGraphProxy(WorkerThreadPool);

        GGameRenderLock = new SimpleLock();
        GRenderRHILock = new SimpleLock();
    }

    void RenderingThread::RenderMain()
    {
        if(GFrameNumberRenderThread.load() - GFrameNumberRHIThread.load() > 1)
        {
            std::unique_lock<std::mutex> lock(GRenderRHILock->mtx);
            GRenderRHILock->cv.wait(lock, []{ return GFrameNumberRenderThread.load() - GFrameNumberRHIThread.load() <= 1; });
        }
        ZoneScoped;
        BusyWaiting(4000); //debug tracy
        
        LOG("Execute render thread in frame: %d with thread: %lu", GFrameNumberRenderThread.load(std::memory_order_acquire), __threadid());
        
        GFrameNumberRenderThread.fetch_add(1, std::memory_order_release);
        GGameRenderLock->cv.notify_all();

        // rhi thread
        auto* testRHIThreadTask = new TTask<RHIThreadTask>(0);
        GRHIThread->PushTask(testRHIThreadTask);
    }

    void RHIThreadTask::RHIMain()
    {
        ZoneScoped;
        BusyWaiting(2000); //debug tracy

        LOG("Execute rhi thread in frame: %d with thread: %lu", GFrameNumberRHIThread.load(std::memory_order_acquire), __threadid());

        GFrameNumberRHIThread.fetch_add(1, std::memory_order_release);
        GRenderRHILock->cv.notify_all();
    }

    int32 EngineMain::FastInit()
    {
        GMalloc = new TMallocMinmalloc();
        ModuleManager::GetInstance()->LoadModule<CoreModule>();
        ModuleManager::GetInstance()->LoadModule<ShaderModule>();
    
        //创建三个线程
        GGameThread = new ThreadProxy();
        GGameThread->CreateSingleThread(4096, "GameThread");

        GRenderThread = new ThreadProxy();
        GRenderThread->CreateSingleThread(4096, "RenderThread");

        GRHIThread = new ThreadProxy();
        GRHIThread->CreateSingleThread(4096, "RHIThread");
        return 0;
    }

    bool EngineMain::RHIInit(EGfxApiType type)
    {
        ShaderModule::GetModule()->InitShaderCompiler(type);
        switch (type)
        {
            case EGfxApiType::D3D12:
            {
                ModuleManager::GetInstance()->LoadModule<TD3D12RHIModule>();
                break;
            }
            case EGfxApiType::D3D11:
            {
                ModuleManager::GetInstance()->LoadModule<TD3D11RHIModule>();
                break;
            }
            case EGfxApiType::Invalid:
                return false;
        }
        
        return true;
    }

    int32 EngineMain::Run()
    {
        auto* testGameThreadTask = new TTask<GameThread>(0);
        GGameThread->PushTask(testGameThreadTask);

        
        GGameThread->WaitForCompletion();
        GRenderThread->WaitForCompletion();
        GRHIThread->WaitForCompletion();
        return 0;
    }
}

#pragma optimize("", on)