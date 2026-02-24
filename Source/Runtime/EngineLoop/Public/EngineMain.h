#pragma once
#include <atomic>
#include "Platform.h"

namespace Thunder
{
	class ENGINELOOP_API EngineMain
    {
    public:
    	EngineMain(){}

		~EngineMain();

        void InitializeEngine();
        void InitWindow(void* hwnd);  // Must be called after InitializeEngine() and before Run()
        void OnWindowResize(uint32 width, uint32 height);
    	bool RHIInit(EGfxApiType type);
		int32 Run();
        void Exit();

		static std::atomic<bool> IsRequestingExit;
		static class IEvent* EngineExitSignal;
		static  bool IsEngineExitRequested()
		{
			return IsRequestingExit.load(std::memory_order_acquire);
		}
    };
}

