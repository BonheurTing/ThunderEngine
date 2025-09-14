
#pragma optimize("", off)
#include "CoreMinimal.h"
#include "EngineMain.h"
#include "HAL/Event.h"


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

    GEngine->FastInit();
    GEngine->Run();

    EngineMain::EngineExitSignal->Wait();

    GEngine->Exit();

    LOG("exit");

    return 0;
}