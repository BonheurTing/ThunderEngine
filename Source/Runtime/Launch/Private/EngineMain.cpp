#include "EngineMain.h"
#include "CoreModule.h"
#include "D3D12RHIModule.h"
#include "D3D11RHIModule.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"
#include "Memory/MallocMinmalloc.h"
#include "Misc/TheadPool.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
    ThreadProxy* GGameThread;
    ThreadProxy* GRenderThread;
    ThreadProxy* GRHIThread;
    std::atomic<uint32> GFrameNumberGameThread = 0;
    bool GIsRequestingExit = false;
    SimpleLock* GThunderEngineLock = nullptr;

    bool IsEngineExitRequested()
    {
        return GIsRequestingExit;
    }

    void GameThreadTask::EngineLoop()
    {
        while( !IsEngineExitRequested() )
        {
            // game thread
            LOG("GameThread Execute Frame: %d", GFrameNumberGameThread.load(std::memory_order_relaxed));
            
            // 物理
            PhysicsTask* TaskPhysics = new PhysicsTask(GFrameNumberGameThread, "PhysicsTask");
            // cull
            CullTask* TaskCull = new CullTask(GFrameNumberGameThread, "CullTask");
            // tick
            TickTask* TaskTick = new TickTask(GFrameNumberGameThread, "TickTask");

            // Task Graph
            TaskGraphManager* TaskGraph = new TaskGraphManager();
            TaskGraph->PushTask(TaskPhysics);
            TaskGraph->PushTask(TaskCull, {TaskPhysics->UniqueId});
            TaskGraph->PushTask(TaskTick, {TaskCull->UniqueId});

            TaskGraph->CreateWorkerThread();
            TaskGraph->WaitForComplete();
            
            // push render command
            FAsyncTask<RenderThreadTask>* testRenderThreadTask = new FAsyncTask<RenderThreadTask>(GFrameNumberGameThread);
            GRenderThread->PushAndExecuteTask(testRenderThreadTask);
            Sleep(100); //是不是不该有
            // rhi
            // frame + 1
            // cv wait (check frame)

            GFrameNumberGameThread.fetch_add(1, std::memory_order_relaxed);
            if (GFrameNumberGameThread >= 10)
            {
                GIsRequestingExit = true;
            }

            // delete todo: use malloc
            //delete TaskGraph;
        }

        GThunderEngineLock->ready = true;
        GThunderEngineLock->cv.notify_all();
        LOG("End GameThread Execute");
    }

    int32 EngineMain::FastInit()
    {
        GMalloc = new TMallocMinmalloc();
        ModuleManager::GetInstance()->LoadModule<CoreModule>();
        ModuleManager::GetInstance()->LoadModule<ShaderModule>();
    
        //创建三个线程
        GGameThread = new ThreadProxy();
        GGameThread->Create(nullptr, 4096, EThreadPriority::Normal);

        GRenderThread = new ThreadProxy();
        GRenderThread->Create(nullptr, 4096, EThreadPriority::Normal);

        GRHIThread = new ThreadProxy();
        GRHIThread->Create(nullptr, 4096, EThreadPriority::Normal);
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
        FAsyncTask<GameThreadTask>* testGameThreadTask = new FAsyncTask<GameThreadTask>(GFrameNumberGameThread);
        GGameThread->DoWork(testGameThreadTask);
    
        return 0;
    }
}

