#pragma once
#include "Container.h"

namespace Thunder
{
	// Usage of these should be replaced with StringCasts.
	#define TCHAR_TO_ANSI(str) (ANSICHAR*)StringCast<ANSICHAR>(static_cast<const TCHAR*>(str)).Get()
	#define ANSI_TO_TCHAR(str) (TCHAR*)StringCast<TCHAR>(static_cast<const ANSICHAR*>(str)).Get()
	#define TCHAR_TO_UTF8(str) (ANSICHAR*)FTCHARToUTF8((const TCHAR*)str).Get()
	#define UTF8_TO_TCHAR(str) (TCHAR*)FUTF8ToTCHAR((const ANSICHAR*)str).Get()
	
	struct CORE_API CommonUtilities
	{
		static int StringToInteger(const String& str, int fallback = 0);
		static bool StringToBool(const String& str, bool fallback = false);
	
	};

}