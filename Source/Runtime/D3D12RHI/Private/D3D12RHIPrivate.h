#pragma once
#include "RHI.h"
#include "d3d12.h"

namespace Thunder
{
	// This is a value that should be tweaked to fit the app, lower numbers will have better performance
	// Titles using many terrain layers may want to set MAX_SRVS to 64 to avoid shader compilation errors. This will have a small performance hit of around 0.1%
	#define MAX_SRVS		64
	#define MAX_SAMPLERS	16
	#define MAX_UAVS		16
	#define MAX_CBS			16

	// This value controls how many root constant buffers can be used per shader stage in a root signature.
	// Note: Using root descriptors significantly increases the size of root signatures (each root descriptor is 2 DWORDs).
	#define MAX_ROOT_CBVS	MAX_CBS
	
	class D3D12RHIFence : public RHIFence
	{
	public:
		D3D12RHIFence(uint64 initValue, ComPtr<ID3D12Fence> const& inFence)
			: RHIFence(initValue), Fence(inFence) {}
		
	private:
        ComPtr<ID3D12Fence> Fence;
	};
}