
#pragma optimize("", off)
#include "CoreMinimal.h"
#include "EngineMain.h"
#include "Misc/TheadPool.h"


namespace Thunder
{
    class ShaderModule;
    class CoreModule;
}

using namespace Thunder;


int main()
{
    //Fast Init
    EngineMain* GEngine = new EngineMain();
    GThunderEngineLock = new SimpleLock();

    GEngine->FastInit();
    GEngine->Run();

    

    //挂起
    {
        std::unique_lock<std::mutex> lock(GThunderEngineLock->mtx);
        GThunderEngineLock->cv.wait(lock, []{ 
            return GIsRequestingExit;
        });
    }

    LOG("exit");

    return 0;
}
#pragma optimize("", on)

/*void PseudoCode()
{
    int CoreNum = FPlatformProcess::GetCoreNum();
    int N = FPlatformProcess::GetLogicProcessorNum();
    // 如果是大小核，创建线程的时候把Game绑定在大核上(创建线程时的参数affinity)
    
    auto SyncWorkers = new ThreadPoolBase(N/2);
    auto AsyncWorkers = new ThreadPoolBase(N/2);

    int Data[1024] {0};
    bool Loaded[1024] {false};

    auto Game = new Thread();
    Game->Dowork():
    {
        int i=16;
        while(i--)
        {
            new AysncTask([&]()
            {
                Loaded[i] = true;
                Data[i] = i*100;
                LOG("第i个模型加载完毕");
            });
            AsyncWorkers->PushTask(AysncTask);
        }

        for(i = 1 -> 1024)
        {
            if(Loaded[i]) LOG(Data[i]);
        }

        TaskGraph([&]()
        {
            new PhysicsTask;
            new CullTask;
            new TickTask;
            SyncWorkers->pushTask;
            SyncWorkers->Submit;
            SyncWorkers->WaitForCompleted;
        });
    }
    auto Render = new Thread([&]()
    {
        new AddMeshBacthTask(sleep(1));
        SyncWorkers->pushTask;
        SyncWorkers->Submit;
        SyncWorkers->WaitForCompleted;
    });
    auto Rhi = new Thread([&]()
    {
        new PopulateCommandList(sleep(1)); //*1024 drawcall
        SyncWorkers->ParallelFor(drawcall, 8, 16);
        LOG("Present");
    });
}*/
