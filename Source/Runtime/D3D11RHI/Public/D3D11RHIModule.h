#pragma once
#include "IDynamicRHI.h"
#include "IRHIModule.h"

namespace Thunder
{
	class TD3D11RHIModule : public IRHIModule
	{
		DECLARE_MODULE_WITH_SUPER(D3D11RHI, TD3D11RHIModule, IRHIModule, D3D11RHI_API)
	public:
		D3D11RHI_API void StartUp() override;
		D3D11RHI_API void ShutDown() override;
	};
}
