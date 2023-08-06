#pragma once
#include "CoreMinimal.h"

class RHIShader
{
public:
	RHIShader() = delete;
	RHIShader(BinaryData inShader) : SharedFunction(inShader) {}
	~RHIShader() {}
private:
	BinaryData SharedFunction;
};
