
#pragma optimize("", off)
#include "CoreMinimal.h"
#include "HAL/Thread.h"
#include "Memory/MallocMinmalloc.h"
#include "Misc/AsyncTask.h"
#include "Misc/FTaskGraphInterface.h"
#include "Misc/TheadPool.h"
#include "Module/ModuleManager.h"

using namespace Thunder;
bool GIsRequestingExit = false;


class GameThreadTask
{
public:
    friend class FAsyncTask<GameThreadTask>;

    int32 FrameData;
    GameThreadTask(): FrameData(0)
    {
    }

    GameThreadTask(int32 InExampleData)
     : FrameData(InExampleData)
    {
    }

    void DoWork()
    {
        LOG("GameThread Task Execute Add 1: %d", FrameData + 1);
    }
};


struct FEngineLoop
{
public:
    int32 Init();
    int32 Tick();
    int32 Exit();
private:
    ThreadProxy* GGameThread;
    ThreadProxy* GRenderThread;
    ThreadProxy* GRHIThread;
    std::atomic<uint32> GFrameNumberGameThread = 0;
};

int32 FEngineLoop::Init()
{
    //创建三个线程
    GGameThread = new ThreadProxy();
    GGameThread->Create(nullptr, 4096, EThreadPriority::Normal);

    GRenderThread = new ThreadProxy();
    GRenderThread->Create(nullptr, 4096, EThreadPriority::Normal);

    GRHIThread = new ThreadProxy();
    GRHIThread->Create(nullptr, 4096, EThreadPriority::Normal);
    return 0;
}

int32 FEngineLoop::Tick()
{
    FAsyncTask<GameThreadTask>* testGameThreadTask = new FAsyncTask<GameThreadTask>(GFrameNumberGameThread);
    GGameThread->DoWork(testGameThreadTask);

    GFrameNumberGameThread.fetch_add(1, std::memory_order_relaxed);
    if (GFrameNumberGameThread >= 10)
    {
        GIsRequestingExit = true;
    }
    
    return 0;
}


FEngineLoop	GEngineLoop;


bool IsEngineExitRequested()
{
    return GIsRequestingExit;
}

int main()
{
    GMalloc = new TMallocMinmalloc();
    
    GEngineLoop.Init();
    
    while( !IsEngineExitRequested() )
    {
        GEngineLoop.Tick();
    }

    return 0;
}
#pragma optimize("", on)

