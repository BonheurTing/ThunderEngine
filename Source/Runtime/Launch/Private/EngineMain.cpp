#pragma optimize("", off)
#include "EngineMain.h"
#include "SimulatedTasks.h"
#include "CoreModule.h"
#include "D3D12RHIModule.h"
#include "D3D11RHIModule.h"
#include "ParallelRendering.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"
#include "Concurrent/TheadPool.h"
#include "Memory/MallocMinmalloc.h"

namespace Thunder
{
    ThreadProxy* GGameThread {};
    ThreadProxy* GRenderThread {};
    ThreadProxy* GRHIThread {};
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

    int32 EngineMain::FastInit()
    {
        GMalloc = new TMallocMinmalloc();
        ModuleManager::GetInstance()->LoadModule<CoreModule>();
    
        //创建三个线程
        GGameThread = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy();
        GGameThread->CreateSingleThread(4096, "GameThread");

        GRenderThread = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy();
        GRenderThread->CreateSingleThread(4096, "RenderThread");

        GRHIThread = new (TMemory::Malloc<ThreadProxy>()) ThreadProxy();
        GRHIThread->CreateSingleThread(4096, "RHIThread");
        return 0;
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
        GGameThread->PushTask(GameThreadTask);

        GGameThread->WaitForCompletion();
        GRenderThread->WaitForCompletion();
        GRHIThread->WaitForCompletion();

        return 0;
    }
}

#pragma optimize("", on)