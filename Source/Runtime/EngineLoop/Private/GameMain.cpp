#include "GameMain.h"
#include "EngineMain.h"
#include "RenderMain.h"
#include "ResourceModule.h"
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
        Scene* newScene = new Scene();
        newScene->SetSceneName("ExampleLevel");

        // Create a root entity (like a game object)
        Entity* rootEntity = newScene->CreateEntity("PlayerCharacter");

        // Add a transform component
        TransformComponent* transform = rootEntity->GetTransform();
        transform->SetPosition(TVector3f(10.0f, 10.0f, 10.0f));
        transform->SetRotation(TVector3f(0.0f, 0.0f, 0.0f));
        transform->SetScale(TVector3f(1.0f, 1.0f, 1.0f));

        // Add a static mesh component
        StaticMeshComponent* meshComp = rootEntity->AddComponent<StaticMeshComponent>();
        // Set mesh and materials would be done here after loading resources

        // Add entity to scene
        newScene->AddRootEntity(rootEntity);

        // Create a child entity
        Entity* childEntity = newScene->CreateEntity("Weapon");
        TransformComponent* childTransform = childEntity->GetTransform();
        childTransform->SetPosition(TVector3f(1.0f, 0.0f, 0.5f));
        rootEntity->AddChild(childEntity);

        

        // Save scene to file
        //NewScene->Save("D:/Game/Levels/ExampleLevel.tmap");

        return newScene;
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

        if (GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) == 5)
        {
            // Informal: load all game resource ( in content directory )
            //StreamableManager::LoadAllAsync();
        }

        if (GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) == 50)
        {
            GameModule::SimulateSceneStreaming();
            //GameModule::SimulateAddMeshToScene();
        }

        if (GFrameState->FrameNumberGameThread.fetch_add(1, std::memory_order_acq_rel) >= EXIT_FRAME_THRESHOLD)
        {
            EngineMain::IsRequestingExit.store(true, std::memory_order_acq_rel);
        }

        // push render command
        const auto renderThreadTask = new (TMemory::Malloc<TTask<RenderingTask>>()) TTask<RenderingTask>(0);
        GRenderScheduler->PushTask(renderThreadTask);

        if (EngineMain::IsRequestingExit.load(std::memory_order_acquire) == true)
        {
            LOG("==================== exit engine ====================");
            EngineMain::EngineExitSignal->Trigger();
            return;
        }

        // push game task for next frame
        auto* gameThreadTask = new (TMemory::Malloc<TTask<GameTask>>()) TTask<GameTask>();
        GGameScheduler->PushTask(gameThreadTask);
    }

    void GameTask::AsyncLoading()
    {
        int frameNum = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire);
        const int LoadingSignal = frameNum % 400;
        if (LoadingSignal == 0)
        {
            uint32 LoadingIndex = frameNum / 400;
            GAsyncWorkers->ParallelFor([this, LoadingIndex](uint32 modelIndex, uint32 dummy, uint32 bundleId)
            {
                ThunderZoneScopedN("AsyncLoading");
                FPlatformProcess::BusyWaiting(100000);
                ModelLoaded[LoadingIndex * 8 + modelIndex] = true;
                ModelData[LoadingIndex * 8 + modelIndex] = static_cast<int>(modelIndex) * 100;
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

        const int32 frameNum = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire);
        // game thread
        LOG("Execute game thread in frame: %d with thread: %lu", frameNum, __threadid());

        // Tick.
        auto const& tickables = GameModule::GetTickables();
        for (auto const& tickable : tickables)
        {
            tickable->Tick();
        }
        
        // physics
        auto* taskPhysics = new (TMemory::Malloc<SimulatedPhysicsTask>()) SimulatedPhysicsTask(frameNum, "PhysicsTask");
        // cull
        auto* taskCull = new (TMemory::Malloc<SimulatedCullTask>()) SimulatedCullTask(frameNum, "CullTask");
        // tick
        auto* taskTick = new (TMemory::Malloc<SimulatedTickTask>()) SimulatedTickTask(frameNum, "TickTask");

        // Task Graph
        TaskGraph->PushTask(taskPhysics);
        TaskGraph->PushTask(taskCull, { taskPhysics });
        TaskGraph->PushTask(taskTick, { taskCull });

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

        // Informal
        TestScene = new Scene(); //TestGenerateExampleScene();
        String fullPath = FileModule::GetResourceContentRoot() + "Map/TestScene.tmap";
        TestScene->LoadAsync(fullPath);
    }

    void GameModule::SimulateAddMeshToScene()
    {
        auto meshs = ResourceModule::GetResourceByClass<StaticMesh>();

        if (!meshs.empty())
        {
            auto scene = GetTestScene();
            auto entities = scene->GetRootEntities();
            for (auto entity : entities)
            {
                TArray<IComponent*> comps = entity->GetComponentByClass<StaticMeshComponent>();
                if (!comps.empty())
                {
                    if (auto stmComp = static_cast<StaticMeshComponent*>(comps[0]))
                    {
                        if (auto mesh = static_cast<StaticMesh*>(meshs[0]))
                        {
                            stmComp->SetMesh(mesh);
                        }
                        return;
                    }
                }
            }
        }
        
    }

    void GameModule::SimulateSceneStreaming()
    {
        GetTestScene()->StreamScene();
    }

    void GameModule::RegisterTickable(ITickable* tickable)
    {
        GetModule()->Tickables.push_back(tickable);
    }

    void GameModule::UnregisterTickable(ITickable* tickable)
    {
        auto tickableIt = std::ranges::find(GetModule()->Tickables, tickable);
        GetModule()->Tickables.erase(tickableIt);
    }
}
