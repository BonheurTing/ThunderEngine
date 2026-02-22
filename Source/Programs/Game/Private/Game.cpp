#include "CoreMinimal.h"
#include "EngineMain.h"
#include "HAL/Event.h"
#include "GameWindow.h"

#if THUNDER_WINDOWS
#include <Windows.h>
#endif

using namespace Thunder;

int main(int argc, char* argv[])
{
    // Handle -waitfordebugger: spin until a debugger attaches
#if THUNDER_WINDOWS
    for (int i = 1; i < argc; ++i)
    {
        if (_stricmp(argv[i], "-waitfordebugger") == 0)
        {
            LOG("Waiting for debugger to attach (PID: %lu)...", GetCurrentProcessId());
            while (!IsDebuggerPresent())
            {
                Sleep(100);
            }
            DebugBreak();
            break;
        }
    }
#endif


    // 1. Create the window
    GameWindow window;
    GameWindowDesc desc;
    desc.Width  = 1920;
    desc.Height = 1080;
    desc.Title  = L"ThunderGame";
    if (!window.Create(desc))
    {
        return -1;
    }

    // 2. Engine initialization
    EngineMain* GEngine = new EngineMain();
    GEngine->InitializeEngine();

    // 3. Pass HWND to RHI layer to create the swapchain
    GEngine->InitWindow(window.GetHWND());

    // 4. Start engine (Game / Render / RHI threads)
    GEngine->Run();

    // 5. Main thread runs message pump until window close or engine exit request
    while (!EngineMain::IsEngineExitRequested())
    {
        if (!window.PumpMessages())
        {
            EngineMain::IsRequestingExit.store(true, std::memory_order_release);
            break;
        }
    }

    // 6. Wait for engine exit signal
    EngineMain::EngineExitSignal->Wait();

    GEngine->Exit();
    window.Destroy();
    delete GEngine;
    return 0;
}
