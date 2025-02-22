#pragma once
#include "Launch.export.h"
#include "Platform.h"

namespace Thunder
{
	extern class ThreadProxy* GGameThread;
	extern ThreadProxy* GRenderThread;
	extern ThreadProxy* GRHIThread;
	
	class LAUNCH_API EngineMain
    {
    public:
    	EngineMain(){}

		~EngineMain();

		int32 FastInit();
    	bool RHIInit(EGfxApiType type);
		int32 Run();

		static bool IsRequestingExit;
		static class IEvent* EngineExitSignal;
		static  bool IsEngineExitRequested()
		{
			return IsRequestingExit;
		}
    };
}

