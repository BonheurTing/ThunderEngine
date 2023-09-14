#pragma once
#include "IRHIModule.h"

namespace Thunder
{
	class D3D12RHI_API TD3D12RHIModule : public IRHIModule
	{
		DECLARE_MODULE_WITH_SUPER(D3D12RHI, TD3D12RHIModule, IRHIModule)
	public:
		void StartUp() override;
		void ShutDown() override {}
	};
}
