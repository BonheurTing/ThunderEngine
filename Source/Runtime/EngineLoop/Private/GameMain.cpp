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
#include "StreamableManager.h"
#include "TestRenderer.h"
#include "FileSystem/FileModule.h"
#include "InputState.h"

#include <cmath>
#include <algorithm>

#define EXIT_FRAME_THRESHOLD 10000

namespace Thunder
{
    static Entity* GFPSCameraEntity = nullptr;

    static void TickFPSCamera(float dt)
    {
        if (!GFPSCameraEntity || !GInputState.RightButtonDown)
        {
            GInputState.MouseDelta = { 0.f, 0.f };
            return;
        }

        auto* tr = GFPSCameraEntity->GetTransform();
        TVector3f pos = tr->GetPosition();
        TVector3f rot = tr->GetRotation();  // Pitch / Yaw / Roll in degrees

        // Mouse rotation
        const float mouseSens = 0.1f;
        rot.Y += GInputState.MouseDelta.X * mouseSens;  // Yaw
        rot.X += GInputState.MouseDelta.Y * mouseSens;  // Pitch
        rot.X = std::clamp(rot.X, -89.f, 89.f);

        // WASD movement along Yaw direction
        const float speed = 5.f * dt;
        float yawRad = rot.Y * 3.14159f / 180.f;
        float fwdX  =  sinf(yawRad);
        float fwdZ  =  cosf(yawRad);
        float rightX =  cosf(yawRad);
        float rightZ = -sinf(yawRad);

        if (GInputState.IsKeyDown('W')) { pos.X += fwdX * speed;   pos.Z += fwdZ * speed; }
        if (GInputState.IsKeyDown('S')) { pos.X -= fwdX * speed;   pos.Z -= fwdZ * speed; }
        if (GInputState.IsKeyDown('A')) { pos.X -= rightX * speed; pos.Z -= rightZ * speed; }
        if (GInputState.IsKeyDown('D')) { pos.X += rightX * speed; pos.Z += rightZ * speed; }
        if (GInputState.IsKeyDown('E')) pos.Y += speed;
        if (GInputState.IsKeyDown('Q')) pos.Y -= speed;

        tr->SetPosition(pos);
        tr->SetRotation(rot);

        LOG("FPS Camera pos=(%.2f, %.2f, %.2f) rot=(%.2f, %.2f, %.2f)",
            pos.X, pos.Y, pos.Z, rot.X, rot.Y, rot.Z);

        // Clear per-frame delta after game thread has consumed it
        GInputState.MouseDelta = { 0.f, 0.f };
    }

    Scene* SimulateGenerateExampleSceneWithEditor()
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
            GAsyncWorkers->ParallelFor([this, LoadingIndex](uint32 modelIndex, uint32 dummy, uint32 threadId)
            {
                ThunderZoneScopedN("AsyncLoading");
                FPlatformProcess::BusyWaiting(100000);
                ModelLoaded[LoadingIndex * 8 + modelIndex] = true;
                ModelData[LoadingIndex * 8 + modelIndex] = static_cast<int>(modelIndex) * 100;
            }, 8, 8);
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

        // First frame: create FPS camera entity
        if (frameNum == 1)
        {
            auto& viewports = GameModule::GetViewports();
            if (!viewports.empty())
            {
                auto* viewport = viewports[0];
                if (viewport)
                {
                    // Create a scene if none exists, or get the first scene from the viewport
                    // For now, create a standalone scene for the camera entity
                    TFunction<class IRenderer*()> defaultRendererFactory = []() -> IRenderer*
                    {
                        return new (TMemory::Malloc<DeferredShadingRenderer>()) DeferredShadingRenderer;
                    };
                    Scene* cameraScene = new Scene(defaultRendererFactory);
                    cameraScene->SetSceneName("FPSCameraScene");
                    Entity* cameraEntity = cameraScene->CreateEntity("FPSCamera");
                    cameraEntity->GetTransform()->SetPosition(TVector3f(0.f, 0.f, -5.f));
                    cameraScene->AddRootEntity(cameraEntity);
                    GFPSCameraEntity = cameraEntity;
                }
            }
        }

        // Tick camera every frame (fixed dt=1/60)
        TickFPSCamera(1.f / 60.f);

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
