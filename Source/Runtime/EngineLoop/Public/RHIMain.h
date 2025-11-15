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

    class RHITask //临时放在这, 也许应该叫 RHIRunner
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

        void SetRenderers(const TArray<class IRenderer*>& inRenderers) { Renderers = inRenderers; }
    private:
        void RHIMain();
        void ExecuteRendererCommands(const IRenderer* renderer);

        TArray<IRenderer*> Renderers;
    };

    
}
