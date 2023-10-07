#pragma once
#include "RHIContext.h"
#include "CoreMinimal.h"
#include "d3dx12.h"

namespace Thunder
{
	class D3D12CommandContext : public RHICommandContext
	{
	public:
		D3D12CommandContext() = delete;
		D3D12CommandContext(ID3D12CommandQueue* inCommandQueue, ID3D12CommandAllocator* inCommandAllocator, ID3D12GraphicsCommandList* inCommandList)
		: CommandQueue(inCommandQueue), CommandAllocator(inCommandAllocator), CommandList(inCommandList) {}

	private:
		ComPtr<ID3D12CommandQueue> CommandQueue;
		ComPtr<ID3D12CommandAllocator> CommandAllocator;
		ComPtr<ID3D12GraphicsCommandList> CommandList;
	};
}
