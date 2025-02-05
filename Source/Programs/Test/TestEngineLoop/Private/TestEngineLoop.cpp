
#pragma optimize("", off)
#include "CoreMinimal.h"
#include "HAL/Thread.h"
#include "Memory/MallocMinmalloc.h"
#include "Misc/AsyncTask.h"
#include "Misc/FTaskGraphInterface.h"
#include "Misc/TheadPool.h"
#include "Module/ModuleManager.h"
#include "CoreModule.h"
#include "ShaderModule.h"

namespace Thunder
{
    class ShaderModule;
    class CoreModule;
}

using namespace Thunder;
bool GIsRequestingExit = false;
std::mutex mtx;
std::condition_variable cv;
bool ready = false;

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
        EngineLoop();
    }
private:
    void EngineLoop();
private:
    
};


bool IsEngineExitRequested()
{
    return GIsRequestingExit;
}
    
ThreadProxy* GGameThread;
ThreadProxy* GRenderThread;
ThreadProxy* GRHIThread;
std::atomic<uint32> GFrameNumberGameThread = 0;

void GameThreadTask::EngineLoop()
{
    while( !IsEngineExitRequested() )
    {
        // game thread
        LOG("GameThread Execute Frame: %d", GFrameNumberGameThread.load(std::memory_order_relaxed));
        // 物理
        // cull
        // tick
        // push render command
            // rhi
                // frame + 1
        // cv wait (check frame)

        GFrameNumberGameThread.fetch_add(1, std::memory_order_relaxed);
        if (GFrameNumberGameThread >= 10)
        {
            GIsRequestingExit = true;
        }
    }

    ready = true;
    cv.notify_all();
    LOG("End GameThread Execute");
}

struct EngineLaunch
{
public:
    int32 Init();
    int32 Run();
    int32 Exit();
private:
};

int32 EngineLaunch::Init()
{
    //Fast Init
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

int32 EngineLaunch::Run()
{
    FAsyncTask<GameThreadTask>* testGameThreadTask = new FAsyncTask<GameThreadTask>(GFrameNumberGameThread);
    GGameThread->DoWork(testGameThreadTask);
    
    return 0;
}

int32 EngineLaunch::Exit()
{
    return 0;
}


EngineLaunch* GEngine = nullptr;



int main()
{
    //Fast Init
    GEngine = new EngineLaunch();

    GEngine->Init();
    GEngine->Run();

    //挂起
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return ready; });   
    }

    LOG("exit");
    
    return 0;
}
#pragma optimize("", on)

