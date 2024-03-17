#pragma once
#include "intrin.h"
#include "Windows/WindowsPlatform.h"

namespace Thunder
{
	struct FPlatformProcess
	{
		
		/**
	 * Tells the processor to pause for implementation-specific amount of time. Is used for spin-loops to improve the speed at 
	 * which the code detects the release of the lock and power-consumption.
	 */
		static void CoreYield()
		{
#if PLATFORM_CPU_X86_FAMILY
			_mm_pause();
#elif PLATFORM_CPU_ARM_FAMILY
#	if !defined(__clang__)
			__yield(); // MSVC
			#	else
			__builtin_arm_yield();
#	endif
#else
#	error Unsupported architecture!
#endif
		}

		
	};
}
