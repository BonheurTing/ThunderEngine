#pragma once
#include <D3D11.h>
#include "RHIResource.h"

namespace Thunder
{
	class D3D11ViewPort : public RHIViewport
	{
	public:
		D3D11ViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
		{
			Viewport.TopLeftX = topLeftX;
			Viewport.TopLeftY = topLeftY;
			Viewport.Width = width;
			Viewport.Height = height;
			Viewport.MinDepth = minDepth;
			Viewport.MaxDepth = maxDepth;
		}
		// swapchain
		_NODISCARD_ const void* GetViewPort() const override { return &Viewport; }
	private:
		D3D11_VIEWPORT Viewport{};
	};
}
