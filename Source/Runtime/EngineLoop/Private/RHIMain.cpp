#pragma optimize("", off)
#include "RHIMain.h"

#include "IDynamicRHI.h"
#include "IRHIModule.h"
#include "RenderResource.h"
#include "RenderContext.h"
#include "RHICommand.h"
#include "Scene.h"
#include "Memory/MallocMinmalloc.h"
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

    void SimulatedPopulateCommandList::DoWork()
    {
        ThunderZoneScopedN("PopulateCommandList");
        FPlatformProcess::BusyWaiting(50);
        Dispatcher->Notify();
    }

    void RHITask::RHIMain()
    {
        /**
         * 1. sync command
         * 2. leit naxt render thread go
         * 3. async command
         * 4. draw command
         **/
        ThunderZoneScopedN("RHIMain");
        uint32 frameNum = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire);
        uint32 currentFrameIndex = frameNum % MAX_FRAME_LAG;
        LOG("Execute rhi thread in frame: %u with thread: %lu", frameNum, __threadid());

        for (auto res : GRHIUpdateAsyncQueue)
        {
            res->Update();
        }
        GRHIUpdateAsyncQueue.clear();
        std::cout << "!!!D3D12CommandContext::Execute Copy with frame index : " << frameNum << std::endl << std::flush;
        IRHIModule::GetModule()->GetCopyCommandContext()->Execute();

        // Parallel recording of rendering commands
        std::cout << "!!!D3D12CommandContext::Reset with frame index : " << frameNum << std::endl << std::flush;
        IRHIModule::GetModule()->ResetCommandContext(currentFrameIndex);
        std::cout << "!!!D3D12CommandContext::Execute with frame index : " << frameNum << std::endl << std::flush;
        for (auto renderer : Renderers)
        {
            CommitRendererCommands(renderer);
        }
        ExecuteRendererCommands();

        const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
        auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
        dispatcher->Promise(1000);
        int i = 1000;
        while (i-- > 0)
        {
            auto* task = new (TMemory::Malloc<SimulatedPopulateCommandList>()) SimulatedPopulateCommandList(dispatcher);
            GSyncWorkers->PushTask(task);
        }
        doWorkEvent->Wait();
        FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
        TMemory::Destroy(dispatcher);

        LOG("Present");

        RHIReleaseResource();
    }

    void RHITask::CommitRendererCommands(const IRenderer* renderer)
    {
        if (!(renderer && renderer->GetFrameGraph()))
        {
            return;
        }
        // Execute D3D12 commands.
        const TArray<RHICommandContextRef>& rhiCommandContexts = IRHIModule::GetModule()->GetRHICommandContexts();

        uint32 frontFrameGraphIndex = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire) % 2;
        auto consolidatedCommands = renderer->GetFrameGraph()->GetCurrentAllCommands(static_cast<int>(frontFrameGraphIndex));
        int commandNum = static_cast<int>(consolidatedCommands.size());
        if (commandNum > 0)
        {
            const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
            auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
            dispatcher->Promise(commandNum);
            GSyncWorkers->ParallelFor([rhiCommandContexts, consolidatedCommands, dispatcher, commandNum](uint32 bundleBegin, uint32 bundleSize, uint32 bundleId) mutable
            {
                RHICommandContext* commandList = rhiCommandContexts[bundleId];
                for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
                {
                    if (index < static_cast<uint32>(commandNum))
                    {
                        size_t currentSize = consolidatedCommands.size();
                        consolidatedCommands.resize(currentSize * 2);
                        consolidatedCommands.resize(currentSize);
                        // consolidatedCommands[index]->TestMember;
                        consolidatedCommands[index]->ExecuteAndDestruct(commandList);
                        dispatcher->Notify();
                    }
                }
            }, commandNum);
            doWorkEvent->Wait();
            FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
            TMemory::Destroy(dispatcher);
        }
    }

    void RHITask::ExecuteRendererCommands()
    {
        const TArray<RHICommandContextRef>& rhiCommandContexts = IRHIModule::GetModule()->GetRHICommandContexts();
        int contextNum = static_cast<int>(rhiCommandContexts.size());
        for (int i = 0; i < contextNum; ++i)
        {
            rhiCommandContexts[i]->Execute(); // Order-preserving
        }
        uint32 fenceIndex = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire) % MAX_FRAME_LAG;
        GDynamicRHI->RHISignalFence(fenceIndex);
    }
}
