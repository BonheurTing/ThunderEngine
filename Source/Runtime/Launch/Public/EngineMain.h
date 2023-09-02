#pragma once
#include "CoreMinimal.h"
#include "Launch.export.h"

namespace Thunder
{
	class LAUNCH_API EngineMain
    {
    public:
    	EngineMain(){}
    	
    	bool RHIInit(EGfxApiType type);
    };
}

