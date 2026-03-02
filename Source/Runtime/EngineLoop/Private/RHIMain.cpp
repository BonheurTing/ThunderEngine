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
#include "DeferredRenderer.h"

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
        ThunderZoneScopedN("RHIMain");
        uint32 frameNum = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire);
        uint32 currentFrameIndex = frameNum % MAX_FRAME_LAG;
        LOG("Execute rhi thread in frame: %u with thread: %lu", frameNum, __threadid());

        RHIBeginFrame(currentFrameIndex);

        IRHIModule::GetModule()->ResetCopyCommandContext_RHI(currentFrameIndex);
        for (auto res : GRHIUpdateAsyncQueue)
        {
            res->Update();
        }
        GRHIUpdateAsyncQueue.clear();
        IRHIModule::GetModule()->GetCopyCommandContext_RHI()->Execute();

        // Parallel recording of rendering commands
        IRHIModule::GetModule()->ResetCommandContext(currentFrameIndex);
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

        RHIPresent();

        RHIReleaseResource_RHIThread();
    }

    void RHITask::CommitRendererCommands(const IRenderer* renderer)
    {
        if (!(renderer && renderer->GetFrameGraph()))
        {
            return;
        }
        // Execute D3D12 commands.
        const TArray<RHICommandContextRef>& rhiCommandContexts = IRHIModule::GetModule()->GetRHICommandContexts();
        uint32 numThread = static_cast<uint32>(rhiCommandContexts.size());

        uint32 frontFrameGraphIndex = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire) % 2;
        auto allCommands = renderer->GetFrameGraph()->GetCurrentAllCommands(static_cast<int>(frontFrameGraphIndex));
        auto passStates = renderer->GetFrameGraph()->GetCurrentPassStates(static_cast<int>(frontFrameGraphIndex));
        int commandNum = static_cast<int>(allCommands.size());
        if (commandNum > 0)
        {
            const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
            auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
            dispatcher->Promise(commandNum);
            GSyncWorkers->ParallelFor([rhiCommandContexts, allCommands, passStates, dispatcher, commandNum](uint32 bundleBegin, uint32 bundleSize) mutable
            {
                uint32 threadId = GetContextId();
                RHICommandContext* commandList = rhiCommandContexts[threadId];

                if (threadId > 0)
                {
                    ExecuteBeginCommandListCommand(passStates, commandList, bundleBegin);
                }

                for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
                {
                    if (index < static_cast<uint32>(commandNum))
                    {
                        allCommands[index]->ExecuteAndDestruct(commandList);
                        dispatcher->Notify();
                    }
                }
            }, commandNum, numThread);
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

    void RHITask::ExecuteBeginCommandListCommand(const TArray<RHIPassState*>& passStates, RHICommandContext* commandList, uint32 firstCommandId)
    {
        if (passStates.empty())
        {
            return;
        }
        // find pass, set render targets
        uint32 passStatIndex = 0;
        // Binary search
        int32 lo = 0;
        int32 hi = static_cast<int32>(passStates.size()) - 1;
        while (lo <= hi)
        {
            int32 mid = lo + (hi - lo) / 2;
            if (passStates[mid]->BeginCommandIndex <= firstCommandId)
            {
                passStatIndex = static_cast<uint32>(mid);
                lo = mid + 1;
            }
            else
            {
                hi = mid - 1;
            }
        }
        auto passStat = passStates[passStatIndex];
        RHIBeginCommandListCommand command(passStat);
        command.Execute(commandList);
    }
}
