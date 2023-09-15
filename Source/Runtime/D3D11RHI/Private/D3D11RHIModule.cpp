#include "D3D11RHIModule.h"
#include "D3D11RHI.h"


namespace Thunder
{
	IMPLEMENT_MODULE(D3D11RHI, TD3D11RHIModule)

	void TD3D11RHIModule::StartUp()
	{
		DynamicRHI = MakeRefCount<D3D11DynamicRHI>();
		GDynamicRHI = DynamicRHI.get();
	}
}
