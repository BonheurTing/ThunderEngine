﻿#include "D3D12RHIModule.h"
#include "D3D12RHI.h"

namespace Thunder
{
	IMPLEMENT_MODULE(D3D12RHI, TD3D12RHIModule)

	void TD3D12RHIModule::StartUp()
	{
		DynamicRHI = new D3D12DynamicRHI();
		GDynamicRHI = DynamicRHI;
	}
}