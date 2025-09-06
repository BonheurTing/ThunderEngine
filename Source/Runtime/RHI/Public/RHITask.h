#pragma once
#include "Concurrent/TaskGraph.h"

namespace Thunder
{
    class SimulatedPopulateCommandList : public ITask
    {
    public:
        SimulatedPopulateCommandList(class TaskDispatcher* inDispatcher)
            : Dispatcher(inDispatcher) {}

        void DoWork() override;

    private:
        TaskDispatcher* Dispatcher{};
    };

    class RHITask //临时放在这
    {
    public:
        friend class TTask<RHITask>;

        int32 FrameData;
        RHITask() : FrameData(0)
        {
        }

        RHITask(int32 InExampleData)
         : FrameData(InExampleData)
        {
        }

        void DoWork()
        {
            RHIMain();
        }
    private:
        void RHIMain();
    };
}
