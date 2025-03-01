#pragma once
#include "d3dx12.h"
#include "RHIResource.h"

namespace Thunder
{
	class D3D12ViewPort : public RHIViewport
	{
	public:
		D3D12ViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
		{
			Viewport.TopLeftX = topLeftX;
			Viewport.TopLeftY = topLeftY;
			Viewport.Width = width;
			Viewport.Height = height;
			Viewport.MinDepth = minDepth;
			Viewport.MaxDepth = maxDepth;
		}
		// swapchain
		// viewport 放在 adapter/device 里
		_NODISCARD_ const void* GetViewPort() const override { return &Viewport; }
	private:
		D3D12_VIEWPORT Viewport{};
	};
}
