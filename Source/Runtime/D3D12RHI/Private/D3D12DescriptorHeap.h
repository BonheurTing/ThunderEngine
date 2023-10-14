#pragma once
#include "CoreMinimal.h"
#include "D3D12RHICommon.h"
#include "d3dx12.h"
#include "RHI.h"
#include "RHIResource.h"

namespace Thunder
{
	class TD3D12DescriptorHeap : public TD3D12DeviceChild
	{
	public:
		TD3D12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptorsPerHeap);

		D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptorHeap(uint32& outHandleOffset);
		void FreeDescriptorHeap(uint32 handleOffset);
	
		FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSlotHandle(uint32 slot) const { return{ CPUBase.ptr + static_cast<SIZE_T>(slot) * DescriptorSize }; }
		FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSlotHandle(uint32 slot) const { return{ GPUBase.ptr + static_cast<SIZE_T>(slot) * DescriptorSize }; }
		
	private:
		static D3D12_DESCRIPTOR_HEAP_DESC CreateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptorsPerHeap)
		{
			D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
			Desc.Type = type;
			Desc.NumDescriptors = numDescriptorsPerHeap;
			Desc.Flags = (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV) ?
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			return Desc;
		}

		inline const uint32 GetDescriptorSize() const { return DescriptorSize; }
		ID3D12DescriptorHeap* GetHeap() { return RootHeap.Get(); }
		const D3D12_DESCRIPTOR_HEAP_DESC& GetDesc() const { return Desc; }
		
		List<uint32> HeapFreeList;
		
		uint32 DescriptorSize;
		const D3D12_DESCRIPTOR_HEAP_DESC Desc;
		ComPtr<ID3D12DescriptorHeap> RootHeap;
		
		D3D12_CPU_DESCRIPTOR_HANDLE CPUBase{};
		D3D12_GPU_DESCRIPTOR_HANDLE GPUBase{};
	};


	/*
	 *  Descriptor View
	 */
	class D3D12DescriptorViewEntry
	{
	public:
		D3D12DescriptorViewEntry(TD3D12DescriptorHeap* heap, uint32 index) : Heap(heap), Index(index) {}
		D3D12DescriptorViewEntry() = delete;

		FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return Heap->GetCPUSlotHandle(Index); }
		FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return Heap->GetGPUSlotHandle(Index); }
	private:
		TD3D12DescriptorHeap* Heap;
		uint32 Index;
	};
	
	class D3D12RHIShaderResourceView : public RHIShaderResourceView, public D3D12DescriptorViewEntry
	{
	public:
		D3D12RHIShaderResourceView(RHIViewDescriptor const& desc, TD3D12DescriptorHeap* heap, uint32 index)
			: RHIShaderResourceView(desc), D3D12DescriptorViewEntry(heap, index) {}
	};

	class D3D12RHIUnorderedAccessView : public RHIUnorderedAccessView, public D3D12DescriptorViewEntry
	{
	public:
		D3D12RHIUnorderedAccessView(RHIViewDescriptor const& desc, TD3D12DescriptorHeap* heap, uint32 index)
			: RHIUnorderedAccessView(desc), D3D12DescriptorViewEntry(heap, index) {}
	};

	class D3D12RHIRenderTargetView : public RHIRenderTargetView, public D3D12DescriptorViewEntry
	{
	public:
		D3D12RHIRenderTargetView(RHIViewDescriptor const& desc, TD3D12DescriptorHeap* heap, uint32 index)
			: RHIRenderTargetView(desc), D3D12DescriptorViewEntry(heap, index) {}
	};

	class D3D12RHIDepthStencilView : public RHIDepthStencilView, public D3D12DescriptorViewEntry
	{
	public:
		D3D12RHIDepthStencilView(RHIViewDescriptor const& desc, TD3D12DescriptorHeap* heap, uint32 index)
			: RHIDepthStencilView(desc), D3D12DescriptorViewEntry(heap, index) {}
	};

	class D3D12RHIConstantBufferView : public RHIConstantBufferView, public D3D12DescriptorViewEntry
	{
	public:
		D3D12RHIConstantBufferView(TD3D12DescriptorHeap* heap, uint32 index)
			: RHIConstantBufferView({}), D3D12DescriptorViewEntry(heap, index) {}
	};

	class D3D12RHISampler : public RHISampler
	{
	public:
		D3D12RHISampler(RHISamplerDescriptor const& desc, int offset)
			: RHISampler(desc), DescripterHeapOffset(offset) {}
		
	private:
		int DescripterHeapOffset;
	};
}
