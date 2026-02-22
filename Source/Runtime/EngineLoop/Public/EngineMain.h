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
        void InitWindow(void* hwnd);  // Must be called after FastInit() and before Run()
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

