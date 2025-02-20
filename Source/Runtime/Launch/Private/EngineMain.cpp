#pragma optimize("", off)
#include "EngineMain.h"
#include "CoreModule.h"
#include "D3D12RHIModule.h"
#include "D3D11RHIModule.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"
#include "Memory/MallocMinmalloc.h"
#include "Misc/ConcurrentBase.h"
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

    ThreadPoolBase* GSyncWorkers {};
    ThreadPoolBase* GAsyncWorkers {};

    bool IsEngineExitRequested()
    {
        return GIsRequestingExit;
    }

    void BusyWaiting(int32 us)
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
        }
    }

    void PhysicsTask::DoWorkInner()
    {
        ZoneScopedN("PhysicsTask");
        BusyWaiting(1000);
        //LOG("Execute physical calculation(data: %d) with thread: %lu", Data, __threadid());
    }

    void CullTask::DoWorkInner()
    {
        ZoneScopedN("CullTask");
        BusyWaiting(1000);
        LOG("Execute clipping calculation(data: %d) with thread: %lu", Data, __threadid());
    }

    void TickTask::DoWorkInner()
    {
        ZoneScopedN("TickTask");
        BusyWaiting(1000);
        LOG("Execute tick calculation(data: %d) with thread: %lu", Data, __threadid());
        if (GFrameNumberGameThread.fetch_add(1, std::memory_order_acq_rel) >= 100000)
        {
            GIsRequestingExit = true;
        }
    }

    void GameThread::Init()
    {
        const auto ThreadNum = FPlatformProcess::NumberOfLogicalProcessors();
        TAssert(GSyncWorkers == nullptr && GAsyncWorkers == nullptr);
        GSyncWorkers = new ThreadPoolBase(ThreadNum > 3 ? (ThreadNum - 3) : ThreadNum, 96 * 1024);
        GAsyncWorkers = new ThreadPoolBase(4, 96 * 1024);

        TaskGraph = new TaskGraphProxy(GSyncWorkers);

        GGameRenderLock = new SimpleLock();
        GRenderRHILock = new SimpleLock();
    }

    void GameThread::EngineLoop()
    {
        AsyncLoading();
        
        while (!IsEngineExitRequested())
        {
            FrameMark;

            // 每帧打印加载情况
            int LoadingCount = 0;
            for (const bool i : ModelLoaded)
            {
                if(!i) LoadingCount++;
            }
            LOG("There are %d models left to load", LoadingCount);

            GameMain();

            // push render command
            const auto testRenderThreadTask = new TTask<RenderingThread>(0);
            GRenderThread->PushTask(testRenderThreadTask);
        }

        LOG("End GameThread Execute");
        GThunderEngineLock->cv.notify_all();
    }

    void GameThread::AsyncLoading()
    {
        GAsyncWorkers->ParallelFor([this](uint32 bundleBegin, uint32 bundleSize)
        {
            for (uint32 i = bundleBegin; i < bundleBegin + bundleSize; i++)
            {
                ZoneScopedN("AsyncLoading");
                ModelLoaded[i] = true;
                ModelData[i] = static_cast<int>(i) * 100;
                // LOG("Model %d is loaded", i);
            }
        }, 1024, 128);
    }

    void GameThread::GameMain()
    {
        ZoneScopedN("GameMain");

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
        TaskGraph->PushTask(TaskCull, { TaskPhysics });
        TaskGraph->PushTask(TaskTick, { TaskCull });

        TaskGraph->Submit();
        
        TaskGraph->Reset();
    }

    void RenderingThread::RenderMain()
    {
        if(GFrameNumberRenderThread.load() - GFrameNumberRHIThread.load() > 1)
        {
            std::unique_lock<std::mutex> lock(GRenderRHILock->mtx);
            GRenderRHILock->cv.wait(lock, []{ return GFrameNumberRenderThread.load() - GFrameNumberRHIThread.load() <= 1; });
        }
        ZoneScopedN("RenderMain");
        
        LOG("Execute render thread in frame: %d with thread: %lu", GFrameNumberRenderThread.load(std::memory_order_acquire), __threadid());

        SimulatingAddingMeshBatch();
        
        GFrameNumberRenderThread.fetch_add(1, std::memory_order_release);
        GGameRenderLock->cv.notify_all();

        // rhi thread
        auto* testRHIThreadTask = new TTask<RHIThreadTask>(0);
        GRHIThread->PushTask(testRHIThreadTask);
    }

    class TaskDispatcher : public IOnCompleted
    {
    public:
        TaskDispatcher(IEvent* inEvent)
            : Event(inEvent)
        {
        }
        void OnCompleted() override
        {
            LOG("TaskCounter OnCompleted");
            Event->Trigger();
        }
    private:
        IEvent* Event{};
    };

    class AddMeshBatchTask : public ITask
    {
    public:
        AddMeshBatchTask(TaskDispatcher* inDispatcher)
            : Dispatcher(inDispatcher) {}

        void DoWork() override
        {
            ZoneScopedN("AddMeshBatch");
            BusyWaiting(50);
            Dispatcher->Notify();
        }
    private:
        TaskDispatcher* Dispatcher{};
    };

    void RenderingThread::SimulatingAddingMeshBatch()
    {
        const auto DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
        auto* Dispatcher = new TaskDispatcher(DoWorkEvent);
        Dispatcher->Promise(1000);
        int i = 1000;
        while (i-- > 0)
        {
            auto* Task = new AddMeshBatchTask(Dispatcher);
            GSyncWorkers->AddQueuedWork(Task);
        }
        DoWorkEvent->Wait();
        FPlatformProcess::ReturnSyncEventToPool(DoWorkEvent);
    }

    class PopulateCommandListTask : public ITask
    {
    public:
        PopulateCommandListTask(TaskDispatcher* inDispatcher)
            : Dispatcher(inDispatcher) {}

        void DoWork() override
        {
            ZoneScopedN("PopulateCommandList");
            BusyWaiting(50);
            Dispatcher->Notify();
        }
    private:
        TaskDispatcher* Dispatcher{};
    };

    void RHIThreadTask::RHIMain()
    {
        ZoneScoped;

        LOG("Execute rhi thread in frame: %d with thread: %lu", GFrameNumberRHIThread.load(std::memory_order_acquire), __threadid());

        const auto DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
        auto* Dispatcher = new TaskDispatcher(DoWorkEvent);
        Dispatcher->Promise(1000);
        int i = 1000;
        while (i-- > 0)
        {
            auto* Task = new PopulateCommandListTask(Dispatcher);
            GSyncWorkers->AddQueuedWork(Task);
        }
        DoWorkEvent->Wait();
        FPlatformProcess::ReturnSyncEventToPool(DoWorkEvent);

        LOG("Present");

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
        auto* testGameThreadTask = new TTask<GameThread>();
        GGameThread->PushTask(testGameThreadTask);

        
        GGameThread->WaitForCompletion();
        GRenderThread->WaitForCompletion();
        GRHIThread->WaitForCompletion();
        return 0;
    }
}

#pragma optimize("", on)