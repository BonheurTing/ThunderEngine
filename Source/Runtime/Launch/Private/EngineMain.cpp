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

    void GameThread::EngineLoop()
    {
        while( !IsEngineExitRequested() )
        {
            GameMain();

            // push render command
            TTask<RenderingThread>* testRenderThreadTask = new TTask<RenderingThread>(GFrameNumberGameThread);
            GRenderThread->PushTask(testRenderThreadTask);
            Sleep(100); //todo 是不是不该有
            // rhi
            // frame + 1
            // cv wait (check frame)

            GFrameNumberGameThread.fetch_add(1, std::memory_order_relaxed);
            if (GFrameNumberGameThread >= 10)
            {
                GIsRequestingExit = true;
            }
        }

        GThunderEngineLock->ready = true;
        GThunderEngineLock->cv.notify_all();
        LOG("End GameThread Execute");
    }

    void GameThread::GameMain()
    {
        // game thread
        LOG("GameThread Execute Frame: %d", GFrameNumberGameThread.load(std::memory_order_relaxed));
            
        // physics
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

        // delete todo: use malloc
        //delete TaskGraph;
    }

    void RenderingThread::RenderMain()
    {
        
        LOG("Execute render thread in frame: %d with thread: %lu", FrameData, __threadid());
        // rhi thread
        TTask<RHIThreadTask>* testRHIThreadTask = new TTask<RHIThreadTask>(GFrameNumberGameThread);
        GRHIThread->PushTask(testRHIThreadTask);
        Sleep(100); //todo 是不是不该有
    }

    void RHIThreadTask::RHIMain()
    {
        LOG("Execute rhi thread in frame: %d with thread: %lu", FrameData, __threadid());
        //GFrameNumberGameThread.fetch_add(1, std::memory_order_relaxed);
    }

    int32 EngineMain::FastInit()
    {
        GMalloc = new TMallocMinmalloc();
        ModuleManager::GetInstance()->LoadModule<CoreModule>();
        ModuleManager::GetInstance()->LoadModule<ShaderModule>();
    
        //创建三个线程
        GGameThread = new ThreadProxy();
        GGameThread->CreateSingleThread(4096, "GameThread");

        GRenderThread = new ThreadProxy();
        GRenderThread->CreateSingleThread(4096, "RenderThread");

        GRHIThread = new ThreadProxy();
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
        TTask<GameThread>* testGameThreadTask = new TTask<GameThread>(GFrameNumberGameThread);
        GGameThread->PushTask(testGameThreadTask);
    
        return 0;
    }
}

