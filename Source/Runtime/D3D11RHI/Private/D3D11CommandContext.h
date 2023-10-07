#pragma once
#include "RHIContext.h"
#include "CoreMinimal.h"
#include "d3d11.h"

namespace Thunder
{
	class D3D11CommandContext : public RHICommandContext
	{
	public:
		D3D11CommandContext() = delete;
		D3D11CommandContext(ID3D11DeviceContext* inContext)
		: DeferredContext(inContext) {}
		
	private:
		ComPtr<ID3D11DeviceContext> DeferredContext;
		//ComPtr<ID3D11CommandList> CommandList;
	};
}

