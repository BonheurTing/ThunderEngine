#include "D3D12DescriptorHeap.h"

#include "Assertion.h"
#include "RHIHelper.h"

namespace Thunder
{
	TD3D12DescriptorHeap::TD3D12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
		uint32 numDescriptorsPerHeap) : TD3D12DeviceChild(device)
		, Desc(CreateDescriptor(type, numDescriptorsPerHeap))
	{
		const auto hr = ParentDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&RootHeap));
		ThrowIfFailed(hr);
		DescriptorSize = ParentDevice->GetDescriptorHandleIncrementSize(Desc.Type);
		CPUBase = RootHeap->GetCPUDescriptorHandleForHeapStart();
		if (type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV && type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		{
			GPUBase = RootHeap->GetGPUDescriptorHandleForHeapStart();
		}

		for (uint32 i = 0; i < numDescriptorsPerHeap; i++)
		{
			HeapFreeList.push_back(i);
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE TD3D12DescriptorHeap::AllocateDescriptorHeap(uint32& outHandleOffset)
	{
		outHandleOffset = HeapFreeList.front();
		HeapFreeList.pop_front();

		return GetCPUSlotHandle(outHandleOffset);
	}

	void TD3D12DescriptorHeap::FreeDescriptorHeap(uint32 handleOffset)
	{
		HeapFreeList.push_front(handleOffset);
	}
}

