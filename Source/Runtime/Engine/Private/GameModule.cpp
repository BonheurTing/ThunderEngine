#include "GameModule.h"
#include "PackageModule.h"
#include "Scene.h"
#include "FileSystem/FileModule.h"
#include "Misc/CoreGlabal.h"
#include "Concurrent/TaskGraph.h"

namespace Thunder
{
    IMPLEMENT_MODULE(Game, GameModule)

    Entity* GFPSCameraEntity = nullptr;

    void GameModule::StartUp()
    {
        // The InitGameThread() cannot be executed because the TaskSchedulerManager is not yet ready
    }

    void GameModule::ShutDown()
    {
        for (auto view : Viewports)
        {
            delete view;
        }
        Viewports.clear();
        TMemory::Free(GameThreadTaskGraph);
    }

    void GameModule::InitGameThread(TFunction<IRenderer*()>& renderFactory)
    {
        GameThreadTaskGraph = new (TMemory::Malloc<TaskGraphProxy>()) TaskGraphProxy(GSyncWorkers);

        GFrameState = new (TMemory::Malloc<FrameState>()) FrameState();

        BaseViewport* viewport = new BaseViewport();
        // test scene 1
        auto scene = new Scene(renderFactory);
        String fullPath = FileModule::GetResourceContentRoot() + "Map/TestScene.tmap";
        scene->LoadAsync(fullPath);
        InitCameraEntity(scene);

        viewport->AddScene(scene);
        Viewports.push_back(viewport);
    }

    void GameModule::InitCameraEntity(Scene* scene)
    {
        Entity* cameraEntity = scene->CreateEntity("FPSCamera");
        cameraEntity->GetTransformComponent()->SetPosition(TVector3f(0.f, 0.f, 0.f));
        cameraEntity->GetTransformComponent()->OnLoaded();
        cameraEntity->AddComponent<CameraComponent>()->OnLoaded();
        scene->AddRootEntity(cameraEntity);
        GFPSCameraEntity = cameraEntity;
    }

    void GameModule::GetProceduralScene(const Scene* inScene)
    {
        auto meshs = PackageModule::GetResourceByClass<StaticMesh>();

        if (!meshs.empty())
        {
            auto entities = inScene->GetRootEntities();
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

    static void SimulateImportAsset()
    {
        String fileName = FileModule::GetProjectRoot() + "\\Resource\\Mesh\\Cube.fbx";
        String destPath = FileModule::GetProjectRoot() + "\\Content\\Mesh\\Cube.tasset";
        PackageModule::Import(fileName, destPath);

        fileName = FileModule::GetProjectRoot() + "\\Resource\\123.fbx";
        destPath = FileModule::GetProjectRoot() + "\\Content\\123.tasset";
        PackageModule::Import(fileName, destPath);

        String texFullPath = FileModule::GetProjectRoot() + "\\Content\\TestPNG.tasset";
        String texSoftPath = PackageModule::CovertFullPathToSoftPath(texFullPath, "TestPNG");
        
        //bool ret = PackageModule::ForceLoadPackage(texSoftPath);
        //TAssert(ret);

        fileName = FileModule::GetProjectRoot() + "\\Resource\\Material\\TestMaterial.mat";
        destPath = FileModule::GetProjectRoot() + "\\Content\\Material\\TestMaterial.tasset";
        PackageModule::Import(fileName, destPath);
    }

    static void SimulateAddMaterialToMesh()
    {

        String matFullPath = FileModule::GetProjectRoot() + "\\Content\\Material\\TestMaterial.tasset";
        String matSoftPath = PackageModule::CovertFullPathToSoftPath(matFullPath, "TestMaterial");
        //bool ret = ResourceModule::ForceLoadBySoftPath(matSoftPath);
        //TAssert(ret);
        GameResource* material = PackageModule::GetResource(matSoftPath);
        TAssert(material != nullptr);

        String meshFullPath = FileModule::GetProjectRoot() + "\\Content\\Mesh\\Cube.tasset";
        String meshSoftPath = PackageModule::CovertFullPathToSoftPath(meshFullPath, "Cube");
        //bool ret = ResourceModule::ForceLoadBySoftPath(meshSoftPath);
        //TAssert(ret);
        GameResource* mesh = PackageModule::GetResource(meshSoftPath);
        TAssert(mesh != nullptr);

        if (auto stMesh = static_cast<StaticMesh*>(mesh))
        {
            stMesh->AddMaterial(static_cast<IMaterial*>(material));

            if (auto pak = static_cast<Package*>(mesh->GetOuter()))
            {
                if (pak->Save())
                {
                    LOG("Successfully add mat to mesh");
                }
            }
        }
    }
}
