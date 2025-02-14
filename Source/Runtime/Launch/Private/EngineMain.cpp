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
    std::atomic<uint32> GFrameNumberRenderThread = 0;
    std::atomic<uint32> GFrameNumberRHIThread = 0;
    std::mutex GFrameMtx;
    std::condition_variable GFrameCv;
    bool GIsRequestingExit = false;
    SimpleLock* GThunderEngineLock = nullptr;

    bool IsEngineExitRequested()
    {
        return GIsRequestingExit;
    }

    void TickTask::DoWork()
    {
        LOG("Execute tick calculation with thread: %lu", __threadid());
        GFrameNumberGameThread.fetch_add(1, std::memory_order_relaxed);
        if (GFrameNumberGameThread >= 10)
        {
            GIsRequestingExit = true;
        }
    }

    void GameThread::EngineLoop()
    {
        while( !IsEngineExitRequested() )
        {
            {
                std::unique_lock<std::mutex> lock(GFrameMtx);
                GFrameCv.wait(lock, []{ return GFrameNumberGameThread - GFrameNumberRenderThread <= 1; });
            }

            GameMain();

            // push render command
            TTask<RenderingThread>* testRenderThreadTask = new TTask<RenderingThread>(GFrameNumberGameThread);
            GRenderThread->PushTask(testRenderThreadTask);
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
        TaskGraph->PushTask(TaskPhysics);
        TaskGraph->PushTask(TaskCull, {TaskPhysics->UniqueId});
        TaskGraph->PushTask(TaskTick, {TaskCull->UniqueId});

        TaskGraph->Submit();
        // TaskGraph->WaitForCompletion();
    }

    void GameThread::Init()
    {
        ThreadPoolBase* WorkerThreadPool = new ThreadPoolBase();
        WorkerThreadPool->Create(8, 96 * 1024);
        TaskGraph = new TaskGraphProxy(WorkerThreadPool);
    }

    void RenderingThread::RenderMain()
    {
        {
            std::unique_lock<std::mutex> lock(GFrameMtx);
            GFrameCv.wait(lock, []{ return GFrameNumberRenderThread - GFrameNumberRHIThread <= 1; });
        }
        
        LOG("Execute render thread in frame: %d with thread: %lu", FrameData, __threadid());
        
        GFrameNumberRenderThread.fetch_add(1, std::memory_order_relaxed);
        GFrameCv.notify_all();

        // rhi thread
        TTask<RHIThreadTask>* testRHIThreadTask = new TTask<RHIThreadTask>(GFrameNumberGameThread);
        GRHIThread->PushTask(testRHIThreadTask);
    }

    void RHIThreadTask::RHIMain()
    {
        LOG("Execute rhi thread in frame: %d with thread: %lu", FrameData, __threadid());
        GFrameNumberRHIThread.fetch_add(1, std::memory_order_relaxed);
        GFrameCv.notify_all();
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

