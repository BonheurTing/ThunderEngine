#include "GameMain.h"
#include "EngineMain.h"
#include "RenderMain.h"
#include "SimulatedTasks.h"
#include "StreamableManager.h"
#include "Memory/MallocMinmalloc.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "HAL/Event.h"
#include "Misc/TraceProfile.h"
#include "Misc/CoreGlabal.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
    void GameTask::Init()
    {
        // init task scheduler
        const auto ThreadNum = FPlatformProcess::NumberOfLogicalProcessors();

        TaskSchedulerManager::InitWorkerThread();

        TaskGraph = new (TMemory::Malloc<TaskGraphProxy>()) TaskGraphProxy(GSyncWorkers);

        GFrameState = new (TMemory::Malloc<FrameState>()) FrameState();

        // init game resource
        StreamableManager::LoadAllAsync();
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
}
