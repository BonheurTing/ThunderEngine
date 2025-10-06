#pragma optimize("", off)
#include "RHIMain.h"

#include "IDynamicRHI.h"
#include "IRHIModule.h"
#include "RenderResource.h"
#include "RenderContext.h"
#include "RHICommand.h"
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

        LOG("Execute rhi thread in frame: %d with thread: %lu", GFrameState->FrameNumberRHIThread.load(), __threadid());

        for (auto res : GRHIUpdateAsyncQueue)
        {
            res->Update();
        }
        GRHIUpdateAsyncQueue.clear();
        if (IRHIModule::GetModule()->TryGetCopyCommandContext())
        {
            IRHIModule::GetModule()->GetCopyCommandContext()->Execute();
        }
        
        // Parallel recording of rendering commands
        ExecuteCommands();
        
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
        TMemory::Destroy(Dispatcher);

        LOG("Present");

        RHIReleaseResource();

        ++GFrameState->FrameNumberRHIThread;
        GFrameState->RenderRHICV.notify_all();
    }

    void RHITask::ExecuteCommands()
    {
        if (!(GTestRenderer && GTestRenderer->GetFrameGraph()))
        {
            return;
        }
        // Execute D3D12 commands.
        const TArray<RHICommandContextRef> rhiCommandContexts = IRHIModule::GetModule()->GetRHICommandContexts();
        int contextNum = static_cast<int>(rhiCommandContexts.size());
        if (contextNum > 1)
        {
            --contextNum;
        }

        int frameIndex = GFrameState->FrameNumberRHIThread % 2;
        auto consolidatedCommands = GTestRenderer->GetFrameGraph()->GetCurrentAllCommands(frameIndex);
        int commandNum = static_cast<int>(consolidatedCommands.size());
        if (commandNum == 0)
        {
            return;
        }
        uint32 taskBundleSize = (commandNum + contextNum - 1) / contextNum;
        //uint32 numTaskBundles = (commandNum + taskBundleSize - 1) / taskBundleSize;

        const auto DoWorkEvent = FPlatformProcess::GetSyncEventFromPool();
        auto* Dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(DoWorkEvent);
        Dispatcher->Promise(commandNum);
        GSyncWorkers->ParallelFor([rhiCommandContexts, consolidatedCommands, Dispatcher](uint32 bundleBegin, uint32 bundleSize, uint32 bundleId) mutable
        {
            uint32 totalNum = static_cast<uint32>(consolidatedCommands.size());
            RHICommandContext* CommandList = rhiCommandContexts[bundleId];
            for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
            {
                if (index < totalNum)
                {
                    size_t currentSize = consolidatedCommands.size();
                    consolidatedCommands.resize(currentSize * 2);
                    consolidatedCommands.resize(currentSize);
                    // consolidatedCommands[index]->TestMember;
                    consolidatedCommands[index]->ExecuteAndDestruct(CommandList);
                    Dispatcher->Notify();
                }
            }
        }, commandNum, taskBundleSize);
        DoWorkEvent->Wait();
        FPlatformProcess::ReturnSyncEventToPool(DoWorkEvent);
        TMemory::Destroy(Dispatcher);

        // Commit.
        for (int i = 0; i < contextNum; ++i)
        {
            rhiCommandContexts[i]->Execute(); // Order-preserving
        }
        GDynamicRHI->RHISignalFence(frameIndex);
    }
}
