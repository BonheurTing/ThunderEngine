#include "D3D12RHIModule.h"
#include "D3D12RHI.h"
#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"

namespace Thunder
{
	IMPLEMENT_MODULE(D3D12RHI, TD3D12RHIModule)

	void TD3D12RHIModule::StartUp()
	{
		IRHIModule::ModuleInstance = this;
		
		DynamicRHI = MakeRefCount<D3D12DynamicRHI>();
		GDynamicRHI = DynamicRHI.get();
	}

	void TD3D12RHIModule::InitD3D12Context(ID3D12Device* InDevice)
	{
		PipelineStateTable = MakeRefCount<TD3D12PipelineStateCache>(InDevice);
		RootSignatureManager = MakeRefCount<TD3D12RootSignatureManager>(InDevice);
	}
}
