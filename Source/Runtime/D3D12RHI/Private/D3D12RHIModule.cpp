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
		InitResourceBindingTier(InDevice);
	}

	void TD3D12RHIModule::InitResourceBindingTier(ID3D12Device* InDevice)
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Caps = {};
		TAssertf(SUCCEEDED(InDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Caps, sizeof(d3d12Caps))), "Failed to check D3D12 feature support.");
		D3D12_RESOURCE_HEAP_TIER resourceHeapTier = d3d12Caps.ResourceHeapTier;
		D3D12_RESOURCE_BINDING_TIER resourceBindingTier = d3d12Caps.ResourceBindingTier;
		(void)(resourceHeapTier);
		InitDescriptorSettings(resourceBindingTier);
	}

	void TD3D12RHIModule::InitDescriptorSettings(D3D12_RESOURCE_BINDING_TIER const& resourceBindingTier)
	{
		DescriptorSettings.SRVDescriptorRangeFlags = (resourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_1) ?
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE :
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		DescriptorSettings.CBVDescriptorRangeFlags = (resourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_2) ?
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE :
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		DescriptorSettings.UAVDescriptorRangeFlags = (resourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_2) ?
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE :
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		DescriptorSettings.SamplerDescriptorRangeFlags = (resourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_1) ?
			D3D12_DESCRIPTOR_RANGE_FLAG_NONE :
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		// We always set the data in an upload heap before calling Set*RootConstantBufferView.
		DescriptorSettings.CBVRootDescriptorFlags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
	}
}
