#pragma once
#include "Platform.h"

struct CORE_API FFileHelper
{
	static bool LoadFileToString(const String& filename, String& outString);
	
};
