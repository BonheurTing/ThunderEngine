#pragma once
#include "Assertion.h"
#include "Templates/RefCountObject.h"

struct ID3D12Device;

namespace Thunder
{
	constexpr uint32 CommonDescriptorHeapSize = 2048;
	constexpr uint32 RTVDescriptorHeapSize = 2048;
	constexpr uint32 DSVDescriptorHeapSize = 2048;
	constexpr uint32 SamplerDescriptorHeapSize = 32;
	
	class TD3D12DeviceChild : public RefCountedObject
	{
	public:
		TD3D12DeviceChild(ID3D12Device* InParent = nullptr) : ParentDevice(InParent) {}

		FORCEINLINE ID3D12Device* GetParentDevice() const
		{
			TAssert(ParentDevice != nullptr);
			return ParentDevice;
		}
		
	protected:
		ID3D12Device* ParentDevice;
	};
}

