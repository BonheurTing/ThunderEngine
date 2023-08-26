#pragma once
#include "Platform.h"

struct CORE_API CommonUtilities
{
	static int StringToInteger(const String& str, int fallback = 0);
	static bool StringToBool(const String& str, bool fallback = false);
	
};
