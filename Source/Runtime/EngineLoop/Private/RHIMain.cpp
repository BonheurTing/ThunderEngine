#include "RHIMain.h"

#include "IRHIModule.h"
#include "RenderResource.h"
#include "Memory/MallocMinmalloc.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "HAL/Event.h"
#include "Misc/CoreGlabal.h"
#include "Misc/TraceProfile.h"
#include "Module/ModuleManager.h"

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

        for (auto res : GRHIUpdateSyncQueue)
        {
            res->UpdateResource_RHIThread();
        }

        IRHIModule::GetModule()->GetCommandContext()->Execute();

        


        
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
