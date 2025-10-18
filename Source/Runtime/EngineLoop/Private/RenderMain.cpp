#pragma optimize("", off)
#include "RenderMain.h"

#include "IDynamicRHI.h"
#include "IRHIModule.h"
#include "RHIMain.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "HAL/Event.h"
#include "Misc/CoreGlabal.h"
#include "Misc/TraceProfile.h"
#include "Module/ModuleManager.h"
#include "TestRenderer.h"

namespace Thunder
{
    void SimulatedAddMeshBatchTask::DoWork()
    {
        ThunderZoneScopedN("AddMeshBatch");
        FPlatformProcess::BusyWaiting(50);
        Dispatcher->Notify();
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

        // Initialize TestRenderer if not already done
        if (!GTestRenderer)
        {
            GTestRenderer = new TestRenderer;
        }

        // Execute FrameGraph rendering pipeline
        GTestRenderer->Setup();
        GTestRenderer->Compile();
        GTestRenderer->Execute();

        SimulatingAddingMeshBatch();
    }

    void RenderingTask::EndRendering()
    {
        // BeginFrame: Wait for fence (3 frames ago) to ensure GPU has completed previous work
        int frameNum = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
        uint32 currentFrameIndex = frameNum % MAX_FRAME_LAG;
        GDynamicRHI->RHIWaitForFrame(currentFrameIndex);

        // begin frame: reset allocator
        int resetIndex = frameNum % MAX_FRAME_LAG;
        IRHIModule::GetModule()->ResetCommandContext(resetIndex);
        // sync with render thread
        for (auto res : GRHIUpdateSyncQueue)
        {
            res->Update();
        }
        GRHIUpdateSyncQueue.clear();

        GFrameState->FrameNumberRenderThread.fetch_add(1, std::memory_order_acq_rel);
        GFrameState->GameRenderCV.notify_all();

        // rhi thread
        auto* RHIThreadTask = new (TMemory::Malloc<TTask<RHITask>>()) TTask<RHITask>(0);
        GRHIScheduler->PushTask(RHIThreadTask);

        // Tick unused frame counters and release long unused render targets
        if (GTestRenderer && GTestRenderer->GetFrameGraph())
        {
            GTestRenderer->GetFrameGraph()->ClearRenderTargetPool();
        }
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
        TMemory::Destroy(Dispatcher);
    }

}
