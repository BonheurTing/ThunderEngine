#pragma once
#include "IDynamicRHI.h"
#include "IRHIModule.h"

namespace Thunder
{
	class D3D11RHI_API TD3D11RHIModule : public IRHIModule
	{
		DECLARE_MODULE_WITH_SUPER(D3D11RHI, TD3D11RHIModule, IRHIModule)
	public:
		void StartUp() override;
		void ShutDown() override {}
	};
}
