#include "GameMain.h"
#include "EngineMain.h"
#include "GameModule.h"
#include "RenderMain.h"
#include "SimulatedTasks.h"
#include "Memory/MallocMinmalloc.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "HAL/Event.h"
#include "Misc/TraceProfile.h"
#include "Misc/CoreGlabal.h"
#include "Module/ModuleManager.h"
#include "Scene.h"
#include "TestRenderer.h"
#include "FileSystem/FileModule.h"

#define EXIT_FRAME_THRESHOLD 100

namespace Thunder
{
    Scene* TestGenerateExampleScene()
    {
        // Create a new scene
        TFunction<class IRenderer*()> defaultRendererFactory = []() -> IRenderer*
        {
            return new (TMemory::Malloc<DeferredShadingRenderer>()) DeferredShadingRenderer; 
        };
        Scene* newScene = new Scene(defaultRendererFactory);
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

        EndGameFrame();
    }

    void GameTask::AsyncLoading()
    {
        uint32 frameNum = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire);
        const uint32 LoadingSignal = frameNum % 400;
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

        const int32 frameNum = static_cast<int32>(GFrameState->FrameNumberGameThread.fetch_add(1, std::memory_order_acq_rel) + 1);
        if (frameNum >= EXIT_FRAME_THRESHOLD)
        {
            EngineMain::IsRequestingExit.store(true, std::memory_order_acq_rel);
        }
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

    void GameTask::EndGameFrame()
    {
        if (GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) == 5)
        {
            // Informal: load all game resource ( in content directory )
            //StreamableManager::LoadAllAsync();
        }

        if (GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) == 50)
        {
            //GameModule::SimulateSceneStreaming();
            //GameModule::SimulateAddMeshToScene();
        }

        // link viewports
        auto& viewports = GameModule::GetViewports();
        TAssert(static_cast<int>(viewports.size()) > 0);
        for (int i = 0; i < static_cast<int>(viewports.size()) - 1; i++)
        {
            viewports[i]->SetNext(viewports[i + 1]);
        }
        // push render command
        WaitForLastRenderFrameEnd();

        const uint32 frameNum = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire);
        GRenderScheduler->PushTask([frameNum]()
        {
            GFrameState->FrameNumberRenderThread.store(frameNum, std::memory_order_release);
            GFrameState->GameRenderCV.notify_all();
        });

        auto renderThreadTask = new (TMemory::Malloc<TTask<RenderingTask>>()) TTask<RenderingTask>();
        (*renderThreadTask)->SetViewport(viewports[0]);
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

    void GameTask::WaitForLastRenderFrameEnd()
    {
        std::unique_lock<std::mutex> lock(GFrameState->GameRenderMutex);
        if (GFrameState->FrameNumberGameThread - GFrameState->FrameNumberRenderThread > 1)
        {
            GFrameState->GameRenderCV.wait(lock, [] {
                return GFrameState->FrameNumberGameThread - GFrameState->FrameNumberRenderThread <= 1;
            });
        }
    }
}
