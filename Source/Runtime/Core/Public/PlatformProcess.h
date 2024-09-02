#pragma once
#include "Windows/WindowsPlatform.h"

#ifndef DEFAULT_NO_THREADING
	#define DEFAULT_NO_THREADING 0
#endif

namespace Thunder
{
	struct FPlatformProcess
	{
		static CORE_API class IEvent* GetSyncEventFromPool(bool bIsManualReset = false);

		static CORE_API void ReturnSyncEventToPool(IEvent* Event);
		
		static bool SupportsMultithreading()
		{
			return !DEFAULT_NO_THREADING;
		}

		static CORE_API class IThread* CreateRunnableThread();
		
		/**
		 * Tells the processor to pause for implementation-specific amount of time. Is used for spin-loops to improve the speed at 
		 * which the code detects the release of the lock and power-consumption.
		 */
		static CORE_API void CoreYield();

		static CORE_API void Sleep( float Seconds );
		
	};
}
