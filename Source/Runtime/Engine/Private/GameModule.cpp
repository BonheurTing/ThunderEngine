#include "GameModule.h"
#include "ResourceModule.h"
#include "Scene.h"
#include "FileSystem/FileModule.h"
#include "Misc/CoreGlabal.h"
#include "Concurrent/TaskGraph.h"

namespace Thunder
{
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

    void GameModule::RegisterTickable(ITickable* tickable)
    {
        GetModule()->Tickables.push_back(tickable);
    }

    void GameModule::UnregisterTickable(ITickable* tickable)
    {
        auto tickableIt = std::ranges::find(GetModule()->Tickables, tickable);
        if (tickableIt != GetModule()->Tickables.end())
        {
            GetModule()->Tickables.erase(tickableIt);
        }
    }

}
