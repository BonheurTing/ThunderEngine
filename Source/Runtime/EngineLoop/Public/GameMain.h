#pragma once
#include "Concurrent/TaskGraph.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
    class GameTask //临时放在这
    {
    public:
        friend class TTask<GameTask>;

        GameTask();
        ~GameTask();

        void DoWork();

        virtual void CustomBeginFrame() {}
        virtual void CustomEndFrame() {}

    private:
        void AsyncLoading();
        void GameMain();
		
    private:
        TaskGraphProxy* TaskGraph {};

        int ModelData[1024] { 0 };
        bool ModelLoaded[1024] { false };
    };

    class ITickable
    {
    public:
        virtual ~ITickable() = default;
        virtual void Tick() = 0;
    };

    class ENGINELOOP_API GameModule : public IModule
    {
        DECLARE_MODULE(Game, GameModule)

    public:
        void StartUp() override;
        void ShutDown() override;

        void InitGameThread();

        static void RegisterTickable(ITickable* Tickable);
        static void UnregisterTickable(ITickable* Tickable);
        static TArray<ITickable*> const& GetTickables() { return GetModule()->Tickables; }

    private:
        friend class GameTask;
        static TaskGraphProxy* GetGameThreadTaskGraph() { return GetModule()->GameThreadTaskGraph; }
        

        TArray<ITickable*> Tickables;
        TaskGraphProxy* GameThreadTaskGraph { nullptr };

        class Scene* TestScene { nullptr };
    };
}

