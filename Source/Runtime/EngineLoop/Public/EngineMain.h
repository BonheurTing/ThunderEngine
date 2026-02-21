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

        void FastInit();
        void InitWindow(void* hwnd);  // Must be called after FastInit() and before Run()
    	bool RHIInit(EGfxApiType type);
		int32 Run();
        void Exit();
		void SimulateEditor();

		static std::atomic<bool> IsRequestingExit;
		static class IEvent* EngineExitSignal;
		static  bool IsEngineExitRequested()
		{
			return IsRequestingExit.load(std::memory_order_acquire);
		}
    };
}

