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

    class RenderingTask
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
            EndRenderer();
        }

        void SetViewport(class BaseViewport* inViewport) { RenderViewport = inViewport; }
        
    private:
        void RenderMain();
        void EndRenderer();

        void SimulatingAddingMeshBatch();

        BaseViewport* RenderViewport { nullptr };
    };

    class EndRenderFrameTask
    {
    public:
        friend class TTask<EndRenderFrameTask>;

        void DoWork()
        {
            EndRenderFrame();
        }
    private:
        void EndRenderFrame();
    };
}
