
#pragma optimize("", off)
#include "CoreMinimal.h"
#include "EngineMain.h"


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
        GThunderEngineLock->cv.wait(lock, []{ return GThunderEngineLock->ready; });
    }

    LOG("exit");

    return 0;
}
#pragma optimize("", on)

