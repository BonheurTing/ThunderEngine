#include "D3D11RHIModule.h"
#include "D3D11RHI.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
	IMPLEMENT_MODULE(D3D11RHI, TD3D11RHIModule)

	void TD3D11RHIModule::StartUp()
	{
		IRHIModule::ModuleInstance = this;
		
		DynamicRHI = MakeRefCount<D3D11DynamicRHI>();
		GDynamicRHI = DynamicRHI.get();

		
	}
}
