#include "RenderCore.h"

#include "RHITask.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "HAL/Event.h"
#include "Misc/CoreGlabal.h"
#include "Misc/TraceProfile.h"
#include "Module/ModuleManager.h"

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

        SimulatingAddingMeshBatch();
        
        ++GFrameState->FrameNumberRenderThread;
        GFrameState->GameRenderCV.notify_all();
    }

    void RenderingTask::EndRendering()
    {
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

}
