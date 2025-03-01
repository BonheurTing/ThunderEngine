#pragma once
#include "CoreMinimal.h"

namespace Thunder
{
	class RHIShader
    {
    public:
    	RHIShader() = delete;
    	RHIShader(BinaryData inShader) : SharedFunction(inShader) {}
    	~RHIShader() {}
    private:
    	BinaryData SharedFunction;
    };
}

