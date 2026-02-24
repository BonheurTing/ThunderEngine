#pragma once
#include "Module/ModuleManager.h"

namespace Thunder
{
    class GameModule : public IModule
    {
        DECLARE_MODULE(Game, GameModule, ENGINE_API)

    public:
        ENGINE_API void StartUp() override;
        ENGINE_API void ShutDown() override;

        ENGINE_API void InitGameThread(TFunction<class IRenderer*()>& renderFactory);
        ENGINE_API static void GetProceduralScene(const class Scene* inScene);
        ENGINE_API static TArray<class BaseViewport*>& GetViewports() { return GetModule()->Viewports; }
        ENGINE_API static BaseViewport* GetMainViewport() { return GetModule()->Viewports.empty() ? nullptr : GetModule()->Viewports[0]; }

        ENGINE_API static void RegisterTickable(class ITickable* tickable);
        ENGINE_API static void UnregisterTickable(ITickable* tickable);
        ENGINE_API static TArray<ITickable*> const& GetTickables() { return GetModule()->Tickables; }

    private:
        ENGINE_API friend class GameTask;
        ENGINE_API static class TaskGraphProxy* GetGameThreadTaskGraph() { return GetModule()->GameThreadTaskGraph; }
        ENGINE_API static void InitCameraEntity(Scene* scene);

        TArray<ITickable*> Tickables;
        TaskGraphProxy* GameThreadTaskGraph { nullptr };

        TArray<class BaseViewport*> Viewports;
    };
}
