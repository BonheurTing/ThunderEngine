#include "D3D11RHIModule.h"
#include "D3D11RHI.h"


namespace Thunder
{
	void TD3D11RHIModule::CreatRHI()
	{
		DynamicRHI = new D3D11DynamicRHI();
	}
}
