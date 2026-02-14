#pragma once
#include "IRHIModule.h"
#include "d3d12.h"
#include "Concurrent/Lock.h"

struct ID3D12Device;
namespace Thunder
{
	class TD3D12PipelineStateCache;
	class TD3D12RootSignatureManager;
	class D3D12PersistentUploadHeapAllocator;

	struct D3D12DescriptorSettings
	{
		D3D12_DESCRIPTOR_RANGE_FLAGS SRVDescriptorRangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		D3D12_DESCRIPTOR_RANGE_FLAGS CBVDescriptorRangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		D3D12_DESCRIPTOR_RANGE_FLAGS UAVDescriptorRangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		D3D12_DESCRIPTOR_RANGE_FLAGS SamplerDescriptorRangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		D3D12_ROOT_DESCRIPTOR_FLAGS CBVRootDescriptorFlags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	};
	
	class D3D12RHI_API TD3D12RHIModule : public IRHIModule
	{
		DECLARE_MODULE_WITH_SUPER(D3D12RHI, TD3D12RHIModule, IRHIModule)
	public:
		void StartUp() override;
		void ShutDown() override;

		void InitD3D12Context(ID3D12Device* InDevice);
		_NODISCARD_ TD3D12PipelineStateCache* GetPipelineStateTable() const { return PipelineStateTable; }
		_NODISCARD_ TD3D12RootSignatureManager* GetRootSignatureManager() const { return RootSignatureManager; }
		void InitResourceBindingTier(ID3D12Device* InDevice);
		void InitDescriptorSettings(D3D12_RESOURCE_BINDING_TIER const& resourceBindingTier);
		D3D12DescriptorSettings const& GetDescriptorSettings() const { return DescriptorSettings; }

		static D3D12PersistentUploadHeapAllocator& GetUploadHeapAllocator();
	private:
		TD3D12PipelineStateCache* PipelineStateTable;
		TD3D12RootSignatureManager* RootSignatureManager;
		D3D12DescriptorSettings DescriptorSettings{};

		// Uniform buffer allocator
		D3D12PersistentUploadHeapAllocator* UploadHeapAllocator;

		TArray<class FTransientUniformBufferAllocator*> TransientUniformBufferAllocators;
		ExclusiveLock TransientUniformBufferAllocatorsCS;
	};
}
