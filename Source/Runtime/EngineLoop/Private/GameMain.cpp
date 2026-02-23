#pragma optimize("", off)
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
#include "DeferredRenderer.h"
#include "FileSystem/FileModule.h"
#include "InputState.h"

#include <cmath>
#include <algorithm>

#define EXIT_FRAME_THRESHOLD 10000

namespace Thunder
{
    ENGINE_API extern Entity* GFPSCameraEntity;

    static void TickFPSCamera(float dt)
    {
        if (!GFPSCameraEntity)
        {
            GInputState.MouseDelta = { 0.f, 0.f };
            return;
        }

        auto* tr = GFPSCameraEntity->GetTransformComponent();
        TVector3f pos = tr->GetPosition();
        TVector3f rot = tr->GetRotation();  // X=Pitch, Y=Yaw, Z=Roll in degrees

        // Mouse rotation (only when right button is held)
        if (GInputState.RightButtonDown)
        {
            const float mouseSens = 0.1f;
            const TVector2f mouseDelta = GInputState.MouseDelta;
            rot.Y += mouseDelta.X * mouseSens;  // Yaw
            rot.X -= mouseDelta.Y * mouseSens;  // Pitch
            rot.X = std::clamp(rot.X, -89.f, 89.f);
        }
        rot.Z = 0.f;  // No roll, Z axis always points up

        // Compute forward and right vectors
        // Coordinate system: X=Forward, Y=Right, Z=Up (same as Unreal Engine)
        constexpr float speed = 300.f;
        const float moveDistance = speed * dt;
        const float yawRad   = rot.Y * DEG_TO_RAD;
        const float pitchRad = rot.X * DEG_TO_RAD;
        const float cosY = cosf(yawRad);
        const float sinY = sinf(yawRad);
        const float cosP = cosf(pitchRad);
        const float sinP = sinf(pitchRad);

        // Forward follows camera pitch and yaw
        const TVector3f forward(cosP * cosY, cosP * sinY, sinP);
        // Right is always horizontal (perpendicular to forward in XY plane)
        const TVector3f right(-sinY, cosY, 0.f);

        if (GInputState.IsKeyDown('W'))
        {
            // todo : TVector3f operator + - * 
            pos.X += forward.X * moveDistance;
            pos.Y += forward.Y * moveDistance;
            pos.Z += forward.Z * moveDistance;
        }
        if (GInputState.IsKeyDown('S'))
        {
            pos.X -= forward.X * moveDistance;
            pos.Y -= forward.Y * moveDistance;
            pos.Z -= forward.Z * moveDistance;
        }
        if (GInputState.IsKeyDown('A'))
        {
            pos.X -= right.X * moveDistance;
            pos.Y -= right.Y * moveDistance;
        }
        if (GInputState.IsKeyDown('D'))
        {
            pos.X += right.X * moveDistance;
            pos.Y += right.Y * moveDistance;
        }
        if (GInputState.IsKeyDown('E'))
        {
            pos.Z += moveDistance;
        }
        if (GInputState.IsKeyDown('Q'))
        {
            pos.Z -= moveDistance;
        }

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
            return new (TMemory::Malloc<DeferredRenderer>()) DeferredRenderer; 
        };
        Scene* newScene = new Scene(defaultRendererFactory);
        newScene->SetSceneName("ExampleLevel");

        // Create a root entity (like a game object)
        Entity* rootEntity = newScene->CreateEntity("PlayerCharacter");

        // Add a transform component
        TransformComponent* transform = rootEntity->GetTransformComponent();
        transform->SetPosition(TVector3f(2.0f, 2.0f, 2.0f));
        transform->SetRotation(TVector3f(0.0f, 0.0f, 0.0f));
        transform->SetScale(TVector3f(1.0f, 1.0f, 1.0f));

        // Add a static mesh component
        StaticMeshComponent* meshComp = rootEntity->AddComponent<StaticMeshComponent>();
        // Set mesh and materials would be done here after loading resources

        // Add entity to scene
        newScene->AddRootEntity(rootEntity);

        // Create a child entity
        Entity* childEntity = newScene->CreateEntity("Weapon");
        TransformComponent* childTransform = childEntity->GetTransformComponent();
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
