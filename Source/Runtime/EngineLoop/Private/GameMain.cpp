#include "GameMain.h"
#include "EngineMain.h"
#include "RenderMain.h"
#include "SimulatedTasks.h"
#include "StreamableManager.h"
#include "Memory/MallocMinmalloc.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "HAL/Event.h"
#include "Misc/TraceProfile.h"
#include "Misc/CoreGlabal.h"
#include "Module/ModuleManager.h"
#include "Scene.h"
#include "FileSystem/FileModule.h"

#define EXIT_FRAME_THRESHOLD 100

namespace Thunder
{
    Scene* TestGenerateExampleScene()
    {
        // Create a new scene
        Scene* NewScene = new Scene();
        NewScene->SetSceneName("ExampleLevel");

        // Create a root entity (like a game object)
        Entity* RootEntity = NewScene->CreateEntity("PlayerCharacter");

        // Add a transform component
        TransformComponent* Transform = RootEntity->AddComponent<TransformComponent>();
        Transform->SetPosition(TVector3f(10.0f, 10.0f, 10.0f));
        Transform->SetRotation(TVector3f(0.0f, 0.0f, 0.0f));
        Transform->SetScale(TVector3f(1.0f, 1.0f, 1.0f));

        // Add a static mesh component
        StaticMeshComponent* MeshComp = RootEntity->AddComponent<StaticMeshComponent>();
        // Set mesh and materials would be done here after loading resources

        // Add entity to scene
        NewScene->AddRootEntity(RootEntity);

        // Create a child entity
        Entity* ChildEntity = NewScene->CreateEntity("Weapon");
        TransformComponent* ChildTransform = ChildEntity->AddComponent<TransformComponent>();
        ChildTransform->SetPosition(TVector3f(1.0f, 0.0f, 0.5f));
        RootEntity->AddChild(ChildEntity);

        

        // Save scene to file
        //NewScene->Save("D:/Game/Levels/ExampleLevel.tmap");

        return NewScene;
    }

    GameTask::GameTask()
    {
        TaskGraph = GameModule::GetGameThreadTaskGraph();
    }

    GameTask::~GameTask()
    {
        TaskGraph = nullptr;
    }

    void GameTask::DoWork()
    {
        ThunderFrameMark;

        AsyncLoading(); //todo: delete

        GameMain();

        if (GFrameState->FrameNumberGameThread.fetch_add(1, std::memory_order_acq_rel) >= EXIT_FRAME_THRESHOLD)
        {
            EngineMain::IsRequestingExit.store(true, std::memory_order_acq_rel);
        }

        // push render command
        const auto RenderThreadTask = new (TMemory::Malloc<TTask<RenderingTask>>()) TTask<RenderingTask>(0);
        GRenderScheduler->PushTask(RenderThreadTask);

        if (EngineMain::IsRequestingExit.load(std::memory_order_acquire) == true)
        {
            LOG("==================== exit engine ====================");
            EngineMain::EngineExitSignal->Trigger();
            return;
        }

        // push game task for next frame
        auto* GameThreadTask = new (TMemory::Malloc<TTask<GameTask>>()) TTask<GameTask>();
        GGameScheduler->PushTask(GameThreadTask);
    }

    void GameTask::AsyncLoading()
    {
        int frameNum = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire);
        const int LoadingSignal = frameNum % 400;
        if (LoadingSignal == 0)
        {
            uint32 LoadingIndex = frameNum / 400;
            GAsyncWorkers->ParallelFor([this, LoadingIndex](uint32 ModelIndex, uint32 dummy, uint32 bundleId)
            {
                ThunderZoneScopedN("AsyncLoading");
                FPlatformProcess::BusyWaiting(100000);
                ModelLoaded[LoadingIndex * 8 + ModelIndex] = true;
                ModelData[LoadingIndex * 8 + ModelIndex] = static_cast<int>(ModelIndex) * 100;
            }, 8, 1);
        }

        // 每帧打印加载情况
        for (int i = 0; i < 1024; i++)
        {
            if (ModelLoaded[i])
            {
                LOG("Model %d is loaded with data: %d", i, ModelData[i]);
            }
        }
    }

    void GameTask::GameMain()
    {
        ThunderZoneScopedN("GameMain");

        {
            std::unique_lock<std::mutex> lock(GFrameState->GameRenderMutex);
            if (GFrameState->FrameNumberGameThread - GFrameState->FrameNumberRenderThread > 1)
            {
                GFrameState->GameRenderCV.wait(lock, [] {
                    return GFrameState->FrameNumberGameThread - GFrameState->FrameNumberRenderThread <= 1;
                });
            }
        }

        const int32 FrameNum = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire);
        // game thread
        LOG("Execute game thread in frame: %d with thread: %lu", FrameNum, __threadid());

        // Tick.
        auto const& tickables = GameModule::GetTickables();
        for (auto const& tickable : tickables)
        {
            tickable->Tick();
        }
        
        // physics
        auto* TaskPhysics = new (TMemory::Malloc<SimulatedPhysicsTask>()) SimulatedPhysicsTask(FrameNum, "PhysicsTask");
        // cull
        auto* TaskCull = new (TMemory::Malloc<SimulatedCullTask>()) SimulatedCullTask(FrameNum, "CullTask");
        // tick
        auto* TaskTick = new (TMemory::Malloc<SimulatedTickTask>()) SimulatedTickTask(FrameNum, "TickTask");

        // Task Graph
        TaskGraph->PushTask(TaskPhysics);
        TaskGraph->PushTask(TaskCull, { TaskPhysics });
        TaskGraph->PushTask(TaskTick, { TaskCull });

        TaskGraph->Submit();
        
        TaskGraph->WaitAndReset();
    }

    IMPLEMENT_MODULE(Game, GameModule)

    void GameModule::StartUp()
    {
        // The InitGameThread() cannot be executed because the TaskSchedulerManager is not yet ready
    }

    void GameModule::ShutDown()
    {
        String fullPath = FileModule::GetResourceContentRoot() + "Map/TestScene.tmap";
        bool ret = TestScene->Save(fullPath);
        TAssertf(ret, "Fail to save scene");
        TMemory::Free(GameThreadTaskGraph);
    }

    void GameModule::InitGameThread()
    {
        // init task scheduler
        TaskSchedulerManager::InitWorkerThread(); // GSyncWorkers Ready

        GameThreadTaskGraph = new (TMemory::Malloc<TaskGraphProxy>()) TaskGraphProxy(GSyncWorkers);

        GFrameState = new (TMemory::Malloc<FrameState>()) FrameState();

        // Informal: load all game resource ( in content directory )
        StreamableManager::LoadAllAsync();

        // Informal
        TestScene = new Scene(); //TestGenerateExampleScene();
        String fullPath = FileModule::GetResourceContentRoot() + "Map/TestScene.tmap";
        TestScene->LoadAsync(fullPath);
    }

    void GameModule::RegisterTickable(ITickable* Tickable)
    {
        GetModule()->Tickables.push_back(Tickable);
    }

    void GameModule::UnregisterTickable(ITickable* Tickable)
    {
        auto tickableIt = std::ranges::find(GetModule()->Tickables, Tickable);
        GetModule()->Tickables.erase(tickableIt);
    }
}
