#pragma once
#include "IRHIModule.h"

namespace Thunder
{
	class TD3D12RHIModule : public IRHIModule
	{
		DECLARE_MODULE_WITH_SUPER(D3D12RHI, TD3D12RHIModule, IRHIModule)
	public:
		void CreatRHI() override;
	};
}
