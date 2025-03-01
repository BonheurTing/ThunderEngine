#include "ParallelRendering.h"
#include "EngineMain.h"
#include "SimulatedTasks.h"
#include "Memory/MallocMinmalloc.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "HAL/Event.h"
#include "Misc/TraceProfile.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
    struct FrameState {
        std::atomic<int> FrameNumberGameThread{0};      // GameThread 的当前帧
        std::atomic<int> FrameNumberRenderThread{0};    // RenderThread 的当前帧
        std::atomic<int> FrameNumberRHIThread{0};       // RHIThread 的当前帧

        std::mutex GameRenderMutex;         // GameThread 和 RenderThread 的同步锁
        std::condition_variable GameRenderCV; // GameThread 和 RenderThread 的条件变量

        std::mutex RenderRHIMutex;          // RenderThread 和 RHIThread 的同步锁
        std::condition_variable RenderRHICV; // RenderThread 和 RHIThread 的条件变量
    };
    static FrameState* GFrameState;
    
    void GameTask::Init()
    {
        const auto ThreadNum = FPlatformProcess::NumberOfLogicalProcessors();

        TaskSchedulerManager::InitWorkerThread();

        TaskGraph = new (TMemory::Malloc<TaskGraphProxy>()) TaskGraphProxy(GSyncWorkers);

        GFrameState = new (TMemory::Malloc<FrameState>()) FrameState();
    }

    void GameTask::EngineLoop()
    {
        Init();

        while (!EngineMain::IsEngineExitRequested())
        {
            ThunderFrameMark;

            AsyncLoading();

            GameMain();

            if (GFrameState->FrameNumberGameThread++ >= 100000)
            {
                EngineMain::IsRequestingExit = true;
            }

            // push render command
            const auto RenderThreadTask = new (TMemory::Malloc<TTask<RenderingTask>>()) TTask<RenderingTask>(0);
            GRenderScheduler->PushTask(RenderThreadTask);
        }

        LOG("End GameThread Execute");
        EngineMain::EngineExitSignal->Trigger();
    }

    void GameTask::AsyncLoading()
    {
        const int LoadingSignal = GFrameState->FrameNumberGameThread % 400;
        if (LoadingSignal == 0)
        {
            uint32 LoadingIndex = GFrameState->FrameNumberGameThread / 400;
            GAsyncWorkers->ParallelFor([this, LoadingIndex](uint32 ModelIndex, uint32 dummy)
            {
                ThunderZoneScopedN("AsyncLoading");
                FPlatformProcess::BusyWaiting(100000);
                ModelLoaded[LoadingIndex * 8 + ModelIndex] = true;
                ModelData[LoadingIndex * 8 + ModelIndex] = static_cast<int>(ModelIndex) * 100;
            }, 8, 1);
        }

        // 每帧打印加载情况
        for (int i = 0; i < 1024; i++)
        {
            if (ModelLoaded[i])
            {
                LOG("Model %d is loaded with data: %d", i, ModelData[i]);
            }
        }
    }

    void GameTask::GameMain()
    {
        ThunderZoneScopedN("GameMain");

        {
            std::unique_lock<std::mutex> lock(GFrameState->GameRenderMutex);
            if (GFrameState->FrameNumberGameThread - GFrameState->FrameNumberRenderThread > 1)
            {
                GFrameState->GameRenderCV.wait(lock, [] {
                    return GFrameState->FrameNumberGameThread - GFrameState->FrameNumberRenderThread <= 1;
                });
            }
        }

        const int32 FrameNum = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire);
        // game thread
        LOG("Execute game thread in frame: %d with thread: %lu", FrameNum, __threadid());

        
        // physics
        auto* TaskPhysics = new (TMemory::Malloc<SimulatedPhysicsTask>()) SimulatedPhysicsTask(FrameNum, "PhysicsTask");
        // cull
        auto* TaskCull = new (TMemory::Malloc<SimulatedCullTask>()) SimulatedCullTask(FrameNum, "CullTask");
        // tick
        auto* TaskTick = new (TMemory::Malloc<SimulatedTickTask>()) SimulatedTickTask(FrameNum, "TickTask");

        // Task Graph
        TaskGraph->PushTask(TaskPhysics);
        TaskGraph->PushTask(TaskCull, { TaskPhysics });
        TaskGraph->PushTask(TaskTick, { TaskCull });

        TaskGraph->Submit();
        
        TaskGraph->WaitAndReset();
    }

    void RenderingTask::RenderMain()
    {
        {
            std::unique_lock<std::mutex> lock(GFrameState->RenderRHIMutex);
            if (GFrameState->FrameNumberRenderThread - GFrameState->FrameNumberRHIThread > 1)
            {
                GFrameState->RenderRHICV.wait(lock, [] {
                    return GFrameState->FrameNumberRenderThread - GFrameState->FrameNumberRHIThread <= 1;
                });
            }
        }

        ThunderZoneScopedN("RenderMain");
        
        LOG("Execute render thread in frame: %d with thread: %lu", GFrameState->FrameNumberRenderThread.load(), __threadid());

        SimulatingAddingMeshBatch();
        
        ++GFrameState->FrameNumberRenderThread;
        GFrameState->GameRenderCV.notify_all();

        // rhi thread
        auto* RHIThreadTask = new (TMemory::Malloc<TTask<RHITask>>()) TTask<RHITask>(0);
        GRHIScheduler->PushTask(RHIThreadTask);
    }

    void RenderingTask::SimulatingAddingMeshBatch()
    {
        const auto DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
        auto* Dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(DoWorkEvent);
        Dispatcher->Promise(1000);
        int i = 1000;
        while (i-- > 0)
        {
            auto* Task = new (TMemory::Malloc<SimulatedAddMeshBatchTask>()) SimulatedAddMeshBatchTask(Dispatcher);
            GSyncWorkers->PushTask(Task);
        }
        DoWorkEvent->Wait();
        FPlatformProcess::ReturnSyncEventToPool(DoWorkEvent);
    }

    void RHITask::RHIMain()
    {
        ThunderZoneScopedN("RHIMain");

        LOG("Execute rhi thread in frame: %d with thread: %lu", GFrameState->FrameNumberRHIThread.load(), __threadid());

        const auto DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
        auto* Dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(DoWorkEvent);
        Dispatcher->Promise(1000);
        int i = 1000;
        while (i-- > 0)
        {
            auto* Task = new (TMemory::Malloc<SimulatedPopulateCommandList>()) SimulatedPopulateCommandList(Dispatcher);
            GSyncWorkers->PushTask(Task);
        }
        DoWorkEvent->Wait();
        FPlatformProcess::ReturnSyncEventToPool(DoWorkEvent);

        LOG("Present");

        ++GFrameState->FrameNumberRHIThread;
        GFrameState->RenderRHICV.notify_all();
    }

}