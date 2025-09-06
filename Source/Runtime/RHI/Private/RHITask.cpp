#include "RHITask.h"
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
        ThunderZoneScopedN("RHIMain");

        LOG("Execute rhi thread in frame: %d with thread: %lu", GFrameState->FrameNumberRHIThread.load(), __threadid());

        for (auto command : GRHISyncQueue)
        {
            command->doWork();
        }
        context->commandqueue->ExecuteCommandLists(commandlists);

        


        
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
