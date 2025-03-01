#include "D3D11RHIModule.h"
#include "D3D11RHI.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
	IMPLEMENT_MODULE(D3D11RHI, TD3D11RHIModule)

	void TD3D11RHIModule::StartUp()
	{
		IRHIModule::ModuleInstance = this;
		
		DynamicRHI = new D3D11DynamicRHI();
		GDynamicRHI = DynamicRHI;
	}

	void TD3D11RHIModule::ShutDown()
	{
		delete DynamicRHI;
		GDynamicRHI = nullptr;
		IRHIModule::ModuleInstance = nullptr;
	}
}
