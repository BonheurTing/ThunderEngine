#pragma once
#include "CoreMinimal.h"
#include "Launch.export.h"

class LAUNCH_API EngineMain
{
public:
	EngineMain(){}
	
	bool RHIInit(ERHIType type);
};
