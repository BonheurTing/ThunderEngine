#include "CoreMinimal.h"
#include "EngineMain.h"
#include "GameModule.h"
#include "HAL/Event.h"
#include "GameWindow.h"
#include "IDynamicRHI.h"
#include "Scene.h"

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


    // Create the window
    GameWindow window;
    GameWindowDesc desc;
    desc.Width  = 1920;
    desc.Height = 1080;
    desc.Title  = L"ThunderGame";
    if (!window.Create(desc))
    {
        return -1;
    }

    // Get actual client area size after window creation
    // (may differ from requested size due to DPI scaling or monitor constraints)
    uint32 actualWidth = window.GetWidth();
    uint32 actualHeight = window.GetHeight();

    // Engine initialization
    EngineMain* GEngine = new EngineMain();
    GEngine->InitializeEngine();

    // Set viewport resolution to match actual window size
    GameModule::GetMainViewport()->SetViewportResolution(TVector2u(actualWidth, actualHeight));

    // Pass HWND to RHI layer to create the swapchain
    GEngine->InitWindow(window.GetHWND());

    // Setup window resize callback
    window.SetResizeCallback([GEngine](uint32 width, uint32 height)
    {
        GEngine->OnWindowResize(width, height);
    });

    // Start engine (Game / Render / RHI threads)
    GEngine->Run();

    // Main thread runs message pump until window close or engine exit request
    while (!EngineMain::IsEngineExitRequested())
    {
        if (!window.PumpMessages())
        {
            EngineMain::IsRequestingExit.store(true, std::memory_order_release);
            break;
        }
    }

    // Wait for engine exit signal
    EngineMain::EngineExitSignal->Wait();

    GEngine->Exit();
    window.Destroy();
    delete GEngine;
    return 0;
}
