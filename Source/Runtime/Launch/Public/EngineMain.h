#pragma once
#include "Launch.export.h"
#include "Platform.h"

namespace Thunder
{
	class LAUNCH_API EngineMain
    {
    public:
    	EngineMain(){}

		~EngineMain();

        void FastInit();
    	bool RHIInit(EGfxApiType type);
		int32 Run();
        void Exit();

		static bool IsRequestingExit;
		static class IEvent* EngineExitSignal;
		static  bool IsEngineExitRequested()
		{
			return IsRequestingExit;
		}
    };
}

