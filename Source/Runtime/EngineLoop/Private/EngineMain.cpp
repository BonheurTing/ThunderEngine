#pragma optimize("", off)
#include "EngineMain.h"
#include "CoreModule.h"
#include "D3D12RHIModule.h"
#include "D3D11RHIModule.h"
#include "GameMain.h"
#include "ResourceModule.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "Memory/MallocMinmalloc.h"
#include "FileSystem/FileModule.h"

namespace Thunder
{
    
    bool EngineMain::IsRequestingExit = false;
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
        ModuleManager::GetInstance()->LoadModule<ResourceModule>();

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

        // setup task scheduler
        TaskSchedulerManager::StartUp();
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
        auto* GameThreadTask = new (TMemory::Malloc<TTask<GameTask>>()) TTask<GameTask>();
        GGameScheduler->PushTask(GameThreadTask);

        GGameScheduler->WaitForCompletionAndThreadExit();
        GRenderScheduler->WaitForCompletionAndThreadExit();
        GRHIScheduler->WaitForCompletionAndThreadExit();

        return 0;
    }

    void EngineMain::Exit()
    {
        TaskSchedulerManager::ShutDown();
        ModuleManager::GetInstance()->UnloadModule<CoreModule>();
        ModuleManager::GetInstance()->UnloadModule<FileModule>();
        ModuleManager::GetInstance()->UnloadModule<ResourceModule>();
    }
}
