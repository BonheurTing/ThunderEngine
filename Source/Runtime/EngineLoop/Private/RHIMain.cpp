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
        uint32 commandNum = static_cast<uint32>(allCommands.size());
        if (commandNum > 0)
        {
            const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
            auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
            dispatcher->Promise(static_cast<int>(commandNum));

            // Split and push tasks.
            const uint32 chunkSize   = (commandNum + numThread - 1) / numThread;
            for (uint32 threadIndex = 0; threadIndex < numThread; ++threadIndex)
            {
                const uint32 chunkBegin = threadIndex * chunkSize;
                if (chunkBegin >= commandNum)
                {
                    break;
                }
                const uint32 chunkEnd = std::min(chunkBegin + chunkSize, commandNum);

                GSyncWorkers->PushTask(static_cast<int>(threadIndex), [rhiCommandContexts, allCommands, passStates, dispatcher, chunkBegin, chunkEnd, threadIndex]()
                {
                    RHICommandContext* commandList = rhiCommandContexts[threadIndex];

                    if (threadIndex > 0)
                    {
                        RHITask::ExecuteBeginCommandListCommand(passStates, commandList, chunkBegin);
                    }

                    for (uint32 index = chunkBegin; index < chunkEnd; ++index)
                    {
                        allCommands[index]->ExecuteAndDestruct(commandList);
                        dispatcher->Notify();
                    }
                });
            }

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
