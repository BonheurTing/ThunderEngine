#pragma once
#include "Platform.h"
#include "Concurrent/Task.h"

namespace Thunder
{
    
    class SimulatedAddMeshBatchTask : public ITask
    {
    public:
        SimulatedAddMeshBatchTask(class TaskDispatcher* inDispatcher)
            : Dispatcher(inDispatcher) {}

        void DoWork() override;

    private:
        TaskDispatcher* Dispatcher{};
    };

    class RenderingTask //临时放在这 后续移到RenderCore
    {
    public:
        friend class TTask<RenderingTask>;

        int32 FrameData;
        RenderingTask() : FrameData(0)
        {
        }

        RenderingTask(int32 InExampleData)
         : FrameData(InExampleData)
        {
        }

        void DoWork()
        {
            RenderMain();
            EndRendering();
        }
    private:
        void RenderMain();

        void EndRendering();
        void SimulatingAddingMeshBatch();
    };
}
