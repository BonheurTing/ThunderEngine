#pragma once
#include "RHI.h"
#include "d3d12.h"

namespace Thunder
{
	class D3D12RHIFence : public RHIFence
	{
	public:
		D3D12RHIFence(uint64 initValue, ComPtr<ID3D12Fence> const& inFence)
			: RHIFence(initValue), Fence(inFence) {}
		
	private:
        ComPtr<ID3D12Fence> Fence;
	};
	
}