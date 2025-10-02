#pragma once

#include "RHI.h"
#include "RHIResource.h"
#include "Vector.h"

namespace Thunder
{
    class RHICommandContext : public RefCountedObject
    {
    public:
        virtual ~RHICommandContext() = default;
        virtual void BeginEvent(const String& eventName) {} //todo pix event runtime
        virtual void EndEvent() {}
    	virtual void SetMarker(const String& markerName) {}

    	// Clear
    	virtual void ClearDepthStencilView(RHIDepthStencilView* dsv, ERHIClearFlags clearFlags, float depthValue, uint8 stencilValue) = 0;
    	virtual void ClearRenderTargetView(RHIRenderTargetView* rtv, TVector4f clearColor) = 0;
    	virtual void ClearState(TRHIPipelineState* pso) = 0;
    	virtual void ClearUnorderedAccessViewUint(RHIResource* resource, TVector4u clearValue) = 0;
    	virtual void ClearUnorderedAccessViewFloat(RHIResource* resource, TVector4f clearValue) = 0;

    	// Set
		virtual void SetIndexBuffer(RHIIndexBufferRef indexBuffer) = 0;
    	virtual void SetPrimitiveTopology(ERHIPrimitive type) = 0;
    	virtual void SetVertexBuffer(uint32 slot, uint32 numViews, RHIVertexBufferRef vertexBuffer) = 0;
    	virtual void SetBlendFactor(TVector4f const& blendFactor) = 0;
    	virtual void SetRenderTarget(uint32 numRT, TArray<RHIRenderTargetView*> rtvs, RHIDepthStencilView* dsv = nullptr) = 0;
    	virtual void SetScissorRects(TArray<RHIRect*> rects) = 0;
    	virtual void SetViewports(TArray<RHIViewport*> viewports) = 0;
    	virtual void SetPipelineState(TRHIPipelineState* pso) = 0;
    	
    	// Copy
    	virtual void CopyBufferRegion(RHIResource* dst, uint64 dstOffset, RHIResource* src, uint64 srcOffset, uint64 numBytes) = 0;
    	virtual void CopyTextureRegion(RHIResource* dst, uint32 dstMip, RHIResource* src, uint32 srcMip, const RHITextureRegion* copyRegion) = 0;
    	virtual void CopyResource(RHIResource* dst, RHIResource* src) = 0;
    	virtual void DiscardResource(RHIResource* resource, TArray<RHIRect> const& rects) = 0;
    	virtual void ResolveSubresource(RHIResource* dst, uint32 dstSubId, RHIResource* src, uint32 srcSubId) = 0; // Resolve multi-sampled texture to non-multi-sampled texture
    	
    	
    	// Draw
		virtual void Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) = 0;
    	virtual void DrawIndexedInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation, int32 baseVertexLocation, uint32 startInstanceLocation) = 0;
    	virtual void DrawInstanced(uint32 vertexCountPerInstance, uint32 instanceCount, uint32 startVertexLocation, uint32 startInstanceLocation) = 0;
    	virtual void Execute() = 0;
    	
    	// Misc
    	virtual void Reset(uint32 index) = 0;
    	virtual void ResourceBarrier() {} //todo
    };

	using RHICommandContextRef = TRefCountPtr<RHICommandContext>;
}
