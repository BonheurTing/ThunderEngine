#pragma once
#include "Module/ModuleManager.h"

namespace Thunder
{
    class ENGINE_API GameModule : public IModule
    {
        DECLARE_MODULE(Game, GameModule)

    public:
        void StartUp() override;
        void ShutDown() override;

        void InitGameThread(TFunction<class IRenderer*()>& renderFactory);
        static void GetProceduralScene(const class Scene* inScene);
        static TArray<class BaseViewport*>& GetViewports() { return GetModule()->Viewports; }

        static void RegisterTickable(class ITickable* tickable);
        static void UnregisterTickable(ITickable* tickable);
        static TArray<ITickable*> const& GetTickables() { return GetModule()->Tickables; }

        static void TestEditor();

    private:
        friend class GameTask;
        static class TaskGraphProxy* GetGameThreadTaskGraph() { return GetModule()->GameThreadTaskGraph; }

        TArray<ITickable*> Tickables;
        TaskGraphProxy* GameThreadTaskGraph { nullptr };

        TArray<class BaseViewport*> Viewports;
    };
}
