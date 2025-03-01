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

		// Clear
		void ClearDepthStencilView(RHIDepthStencilView* dsv, ERHIClearFlags clearFlags, float depthValue, uint8 stencilValue) override {}
		void ClearRenderTargetView(RHIRenderTargetView* rtv, TVector4f clearColor) override {}
		void ClearState(TRHIPipelineState* pso) override {}
		void ClearUnorderedAccessViewUint(RHIResource* resource, TVector4u clearValue) override {}
		void ClearUnorderedAccessViewFloat(RHIResource* resource, TVector4f clearValue) override {}
		
		// Set
		void SetIndexBuffer(RHIIndexBufferRef indexBuffer) override {}
		void SetPrimitiveTopology(ERHIPrimitive type) override {}
		void SetVertexBuffer(uint32 slot, uint32 numViews, RHIVertexBufferRef vertexBuffer) override {}
		void SetBlendFactor(TVector4f const& blendFactor) override {}
		void SetRenderTarget(uint32 numRT, TArray<RHIRenderTargetView*> rtvs, RHIDepthStencilView* dsv = nullptr) override {}
		void SetScissorRects(TArray<RHIRect*> rects) override {}
		void SetViewports(TArray<RHIViewport*> viewports) override {}
		void SetPipelineState(TRHIPipelineState* pso) override {}
    	
		// Copy
		void CopyBufferRegion(RHIResource* dst, uint64 dstOffset, RHIResource* src, uint64 srcOffset, uint64 numBytes) override {}
		void CopyTextureRegion(RHIResource* dst, uint32 dstMip, RHIResource* src, uint32 srcMip, const RHITextureCopyRegion* copyRegion) override {}
		void CopyResource(RHIResource* dst, RHIResource* src) override {}
		void DiscardResource(RHIResource* resource, TArray<RHIRect> const& rects) override {}
		void ResolveSubresource(RHIResource* dst, uint32 dstSubId, RHIResource* src, uint32 srcSubId) override {}
    	
		// Draw
		void Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) override {}
		void DrawIndexedInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation, int32 baseVertexLocation, uint32 startInstanceLocation) override {}
		void DrawInstanced(uint32 vertexCountPerInstance, uint32 instanceCount, uint32 startVertexLocation, uint32 startInstanceLocation) override {}
    	
		// Misc
		void Close() override {}
		void Reset() override {}
		
	private:
		ComPtr<ID3D11DeviceContext> DeferredContext;
		//ComPtr<ID3D11CommandList> CommandList;
	};
}

