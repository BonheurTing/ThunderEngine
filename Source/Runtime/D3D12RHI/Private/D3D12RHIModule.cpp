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
		
		DynamicRHI = new D3D12DynamicRHI();
		GDynamicRHI = DynamicRHI;
	}

	void TD3D12RHIModule::ShutDown()
	{
		if (DynamicRHI)
		{
			delete DynamicRHI;
			GDynamicRHI = nullptr;
		}
		IRHIModule::ModuleInstance = nullptr;

		if (PipelineStateTable)
		{
			delete PipelineStateTable;
			PipelineStateTable = nullptr;
		}

		if (RootSignatureManager)
		{
			delete RootSignatureManager;
			RootSignatureManager = nullptr;
		}
	}

	void TD3D12RHIModule::InitD3D12Context(ID3D12Device* InDevice)
	{
		PipelineStateTable = new TD3D12PipelineStateCache(InDevice);
		RootSignatureManager = new TD3D12RootSignatureManager(InDevice);
	}
}
