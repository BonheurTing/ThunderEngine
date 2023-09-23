#pragma once
struct ID3D12Device;

namespace Thunder
{
	
	class TD3D12DeviceChild
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

