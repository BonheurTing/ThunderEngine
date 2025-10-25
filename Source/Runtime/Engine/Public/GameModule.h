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

        void InitGameThread();
        static void SimulateAddMeshToScene();
        static class Scene* GetTestScene() { return GetModule()->TestScene; }

        static void RegisterTickable(class ITickable* tickable);
        static void UnregisterTickable(ITickable* tickable);
        static TArray<ITickable*> const& GetTickables() { return GetModule()->Tickables; }

    private:
        friend class GameTask;
        static class TaskGraphProxy* GetGameThreadTaskGraph() { return GetModule()->GameThreadTaskGraph; }

        TArray<ITickable*> Tickables;
        TaskGraphProxy* GameThreadTaskGraph { nullptr };

        Scene* TestScene { nullptr };
    };
}
