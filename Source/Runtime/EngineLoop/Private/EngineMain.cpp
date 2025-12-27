#pragma optimize("", off)
#include "EngineMain.h"
#include "CoreModule.h"
#include "D3D12RHIModule.h"
#include "D3D11RHIModule.h"
#include "GameMain.h"
#include "GameModule.h"
#include "PackageModule.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"
#include "TestRenderer.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "Memory/MallocMinmalloc.h"
#include "FileSystem/FileModule.h"

namespace Thunder
{
    
    std::atomic<bool> EngineMain::IsRequestingExit = false;
    IEvent* EngineMain::EngineExitSignal = FPlatformProcess::GetSyncEventFromPool();

    EngineMain::~EngineMain()
    {
        if (EngineExitSignal)
        {
            FPlatformProcess::ReturnSyncEventToPool(EngineExitSignal);
            EngineExitSignal = nullptr;
        }
    }

    void EngineMain::FastInit()
    {
        // setup module
        ModuleManager::GetInstance()->LoadModule<CoreModule>();
        ModuleManager::GetInstance()->LoadModule<FileModule>();
        ModuleManager::GetInstance()->LoadModule<ShaderModule>();
        ModuleManager::GetInstance()->LoadModule<PackageModule>();
        ModuleManager::GetInstance()->LoadModule<GameModule>();

        // setup resource map
        PackageModule::InitResourcePathMap();

        // setup rhi
        String configRHIType = GConfigManager->GetConfig("BaseEngine")->GetString("RHI");
        EGfxApiType rhiType = EGfxApiType::Invalid;
        if (configRHIType == "DX11")
        {
            rhiType = EGfxApiType::D3D11;
        }
        else if (configRHIType == "DX12")
        {
            rhiType = EGfxApiType::D3D12;
        }
        else
        {
            TAssertf(false, "Invalid RHI Type");
        }
        if (!RHIInit(rhiType))
        {
            return;
        }
        std::cout << IRHIModule::GetModule()->GetName().c_str() << std::endl;

        RHICreateDevice();
        IRHIModule::GetModule()->InitCommandContext();

        // setup task scheduler: parallel render thread, worker thread
        TaskSchedulerManager::StartUp();

        // setup shader archive
        ShaderModule::InitShaderMap();

        TFunction<class IRenderer*()> defaultRendererFactory = [this]() -> IRenderer*
        {
            return new (TMemory::Malloc<DeferredShadingRenderer>()) DeferredShadingRenderer; 
        };
        GameModule::GetModule()->InitGameThread(defaultRendererFactory); //bonheur test
    }

    bool EngineMain::RHIInit(EGfxApiType type)
    {
        ShaderModule::GetModule()->InitShaderCompiler(type);
        switch (type)
        {
            case EGfxApiType::D3D12:
            {
                ModuleManager::GetInstance()->LoadModule<TD3D12RHIModule>();
                break;
            }
            case EGfxApiType::D3D11:
            {
                ModuleManager::GetInstance()->LoadModule<TD3D11RHIModule>();
                break;
            }
            case EGfxApiType::Invalid:
                return false;
        }
        
        return true;
    }

    int32 EngineMain::Run()
    {
        auto* gameThreadTask = new (TMemory::Malloc<TTask<GameTask>>()) TTask<GameTask>();
        GGameScheduler->PushTask(gameThreadTask);
        /**
         * plan a: the main thread push a game task.
         *      then, at the end of the game task's function (DoWork), the game task submits the next game task
         * plan b: the main thread push a game task within a while loop
         *      and uses a signal to wait for the game task's function (DoWork) to complete.
         *      at the end of DoWork, this signal is triggered.
         * at last, i choose plan a because main thread may handle IO operations (such as keyboard and mouse input)
         *      that require immediate response in the future,
         *      while the "while expression" needs to wait for the completion of the game task before responding.
        **/

        // may be handle IO operations (such as keyboard and mouse input)

        GGameScheduler->WaitForCompletionAndThreadExit();
        GRenderScheduler->WaitForCompletionAndThreadExit();
        GRHIScheduler->WaitForCompletionAndThreadExit();

        return 0;
    }

    void EngineMain::Exit()
    {
        TaskSchedulerManager::ShutDown();
        ModuleManager::GetInstance()->UnloadModule<GameModule>();
        ModuleManager::GetInstance()->UnloadModule<PackageModule>();
        ModuleManager::GetInstance()->UnloadModule<ShaderModule>();
        ModuleManager::GetInstance()->UnloadModule<FileModule>();
        ModuleManager::GetInstance()->UnloadModule<CoreModule>();
    }

    void EngineMain::SimulateEditor()
    {
        GameModule::TestEditor();
    }
}
