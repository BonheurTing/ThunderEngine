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
#define WITH_EDITOR 0

int main()
{
    //Fast Init
    EngineMain* GEngine = new EngineMain();

    GEngine->FastInit();

#if WITH_EDITOR
    GEngine->SimulateEditor();
#else
    GEngine->Run();
    EngineMain::EngineExitSignal->Wait();
#endif
    GEngine->Exit();

    LOG("exit");

    return 0;
}