#include "GameModule.h"
#include "ResourceModule.h"
#include "Scene.h"
#include "StreamableManager.h"
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
        for (auto view : Viewports)
        {
            delete view;
        }
        Viewports.clear();
        TMemory::Free(GameThreadTaskGraph);
    }

    void GameModule::InitGameThread(TFunction<IRenderer*()>& renderFactory)
    {
        // 12.20 SimulateTest: load tassel in order：png->material->mesh
        {
            uint32 A, B, C, D;
            TGuid resGuid;
            if (sscanf("28064C42-469B8D63-A19B4897-A9A8A160", "%08X-%08X-%08X-%08X", &A, &B, &C, &D) == 4)
            {
                resGuid = TGuid(A, B, C, D);
            }
            GameResource* tex = ResourceModule::LoadSync(resGuid, true);
            tex->OnResourceLoaded();

            if (sscanf("F8432BAB-4EE48DCE-C0D0A6B5-FCA7DEA3", "%08X-%08X-%08X-%08X", &A, &B, &C, &D) == 4)
            {
                resGuid = TGuid(A, B, C, D);
            }
            GameResource* mat = ResourceModule::LoadSync(resGuid, true);
            mat->OnResourceLoaded();

            if (sscanf("3A8C62D8-4FE31B09-66906FB7-BAF089A9", "%08X-%08X-%08X-%08X", &A, &B, &C, &D) == 4)
            {
                resGuid = TGuid(A, B, C, D);
            }
            GameResource* mesh = ResourceModule::LoadSync(resGuid, true);
            mesh->OnResourceLoaded();
            if (auto stMesh = static_cast<StaticMesh*>(mesh))
            {
                stMesh->AddMaterial(static_cast<IMaterial*>(mat));
            }
        }
        

        GameThreadTaskGraph = new (TMemory::Malloc<TaskGraphProxy>()) TaskGraphProxy(GSyncWorkers);

        GFrameState = new (TMemory::Malloc<FrameState>()) FrameState();

        BaseViewport* viewport = new BaseViewport();
        // test scene 1
        auto scene = new Scene(renderFactory);
        String fullPath = FileModule::GetResourceContentRoot() + "Map/TestScene.tmap";
        scene->LoadAsync(fullPath);
        viewport->AddScene(scene);

        // test two scene
        // auto scene2 = new Scene(renderFactory);
        // String fullPath2 = FileModule::GetResourceContentRoot() + "Map/TestScene2.tmap";
        // scene2->LoadAsync(fullPath2);
        // viewport->AddScene(scene2);

        Viewports.push_back(viewport);

        // test two viewport
        // BaseViewport* viewport2 = new BaseViewport();
        // auto scene2 = new Scene(renderFactory);
        // scene2->LoadAsync(fullPath);
        // viewport2->AddScene(scene2);
        // auto scene3 = new Scene(renderFactory);
        // scene3->LoadAsync(fullPath);
        // viewport2->AddScene(scene3);
        // Viewports.push_back(viewport2);
    }

    void GameModule::GetProceduralScene(const Scene* inScene)
    {
        auto meshs = ResourceModule::GetResourceByClass<StaticMesh>();

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
        ResourceModule::ForceImport(fileName, destPath);

        fileName = FileModule::GetProjectRoot() + "\\Resource\\123.fbx";
        destPath = FileModule::GetProjectRoot() + "\\Content\\123.tasset";
        ResourceModule::ForceImport(fileName, destPath);

        String texFullPath = FileModule::GetProjectRoot() + "\\Content\\TestPNG.tasset";
        String texSoftPath = ResourceModule::CovertFullPathToSoftPath(texFullPath, "TestPNG");
        bool ret = ResourceModule::ForceLoadBySoftPath(texSoftPath);
        TAssert(ret);

        fileName = FileModule::GetProjectRoot() + "\\Resource\\Material\\TestMaterial.mat";
        destPath = FileModule::GetProjectRoot() + "\\Content\\Material\\TestMaterial.tasset";
        ResourceModule::ForceImport(fileName, destPath);
    }

    static void SimulateAddMaterialToMesh()
    {

        String matFullPath = FileModule::GetProjectRoot() + "\\Content\\Material\\TestMaterial.tasset";
        String matSoftPath = ResourceModule::CovertFullPathToSoftPath(matFullPath, "TestMaterial");
        //bool ret = ResourceModule::ForceLoadBySoftPath(matSoftPath);
        //TAssert(ret);
        GameObject* material = ResourceModule::GetResource(matSoftPath);
        TAssert(material != nullptr);

        String meshFullPath = FileModule::GetProjectRoot() + "\\Content\\Mesh\\Cube.tasset";
        String meshSoftPath = ResourceModule::CovertFullPathToSoftPath(meshFullPath, "Cube");
        //bool ret = ResourceModule::ForceLoadBySoftPath(meshSoftPath);
        //TAssert(ret);
        GameObject* mesh = ResourceModule::GetResource(meshSoftPath);
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


    void GameModule::TestEditor()
    {
        // test png->material->mesh
        {
            //SimulateImportAsset();
            //SimulateAddMaterialToMesh();
        }

        // print all resources
        StreamableManager::ForceLoadAllResources();
    }
}
