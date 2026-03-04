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

    class RHITask
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
        void CommitRendererCommands(const IRenderer* renderer);
        void ExecuteRendererCommands();
        static void ExecuteBeginCommandListCommand(const TArray<struct RHIPassState*>& passStates, class RHICommandContext* commandList, uint32 firstCommandId);

        TArray<IRenderer*> Renderers;
    };

    
}
