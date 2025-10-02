#include "IRHIModule.h"

#include "IDynamicRHI.h"
#include "PlatformProcess.h"

namespace Thunder
{
	IRHIModule* IRHIModule::ModuleInstance = nullptr;

	void IRHIModule::InitCommandContext()
	{
		const int32 num = FPlatformProcess::NumberOfLogicalProcessors();
		int32 contextNum = num > 3 ? (num - 3) : num;
		CommandContexts.resize(contextNum);
		for (int32 i = 0; i < contextNum; ++i)
		{
			CommandContexts[i] = RHICreateCommandContext();
		}
	}

	void IRHIModule::ResetCommandContext(uint32 index) const
	{
		for (const auto& context : CommandContexts)
		{
			context->Reset(index);
		}
	}
}
