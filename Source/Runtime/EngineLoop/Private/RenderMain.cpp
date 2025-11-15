#pragma optimize("", off)
#include "RenderMain.h"

#include "IDynamicRHI.h"
#include "IRHIModule.h"
#include "RHIMain.h"
#include "Scene.h"
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
        ThunderZoneScopedN("RenderMain");

        LOG("Execute render thread in frame: %d with thread: %lu", GFrameState->FrameNumberRenderThread.load(), __threadid());

        for (auto scene : RenderViewport->GetScenes())
        {
            auto renderer = scene->GetRenderer();
            TAssert(renderer != nullptr);

            // Execute FrameGraph rendering pipeline
            renderer->Setup();
            renderer->Compile();
            renderer->Cull();
            renderer->Execute();

            SimulatingAddingMeshBatch();
        }
    }

    void RenderingTask::EndRenderer()
    {
        // BeginFrame: Wait for fence (3 frames ago) to ensure GPU has completed previous work
        int frameNum = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
        uint32 currentFrameIndex = frameNum % MAX_FRAME_LAG;
        GDynamicRHI->RHIWaitForFrame(currentFrameIndex);

        // Wait for rhi thread.
        WaitForLastRHIFrameEnd();

        // begin frame: reset allocator
		std::cout << " D3D12CommandContext::Reset copy with frame index : " << frameNum << std::endl << std::flush;
        IRHIModule::GetModule()->ResetCopyCommandContext(currentFrameIndex);
        // sync with render thread
        for (auto res : GRHIUpdateSyncQueue)
        {
            res->Update();
        }
        GRHIUpdateSyncQueue.clear();

        for (auto scene : RenderViewport->GetScenes())
        {
            auto renderer = scene->GetRenderer();
            TAssert(renderer != nullptr);
            renderer->EndRenderFrame();
        }

        // rhi thread
        GRHIScheduler->PushTask([frameNum]()
        {
            GFrameState->FrameNumberRHIThread.store(static_cast<int>(frameNum), std::memory_order_release);
            GFrameState->RenderRHICV.notify_all();
        });
        TArray<IRenderer*> renderers;
        RenderViewport->GetRenderers(renderers);
        auto* RHIThreadTask = new (TMemory::Malloc<TTask<RHITask>>()) TTask<RHITask>();
        (*RHIThreadTask)->SetRenderers(renderers);
        GRHIScheduler->PushTask(RHIThreadTask);

        // render next viewport
        if (RenderViewport->GetNext())
        {
            auto renderThreadTask = new (TMemory::Malloc<TTask<RenderingTask>>()) TTask<RenderingTask>();
            (*renderThreadTask)->SetViewport(RenderViewport->GetNext());
            GRenderScheduler->PushTask(renderThreadTask);
        }
        else
        {
            auto endRenderFrameTask = new (TMemory::Malloc<TTask<EndRenderFrameTask>>()) TTask<EndRenderFrameTask>();
            GRenderScheduler->PushTask(endRenderFrameTask);
        }
    }

    void RenderingTask::WaitForLastRHIFrameEnd()
    {
        std::unique_lock<std::mutex> lock(GFrameState->RenderRHIMutex);
        if (GFrameState->FrameNumberRenderThread - GFrameState->FrameNumberRHIThread > 1)
        {
            GFrameState->RenderRHICV.wait(lock, [] {
                return GFrameState->FrameNumberRenderThread - GFrameState->FrameNumberRHIThread <= 1;
            });
        }
    }

    void RenderingTask::SimulatingAddingMeshBatch()
    {
        const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
        auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
        dispatcher->Promise(1000);
        int i = 1000;
        while (i-- > 0)
        {
            auto* Task = new (TMemory::Malloc<SimulatedAddMeshBatchTask>()) SimulatedAddMeshBatchTask(dispatcher);
            GSyncWorkers->PushTask(Task);
        }
        doWorkEvent->Wait();
        FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
        TMemory::Destroy(dispatcher);
    }

    void EndRenderFrameTask::EndRenderFrame()
    {
        
    }

}
