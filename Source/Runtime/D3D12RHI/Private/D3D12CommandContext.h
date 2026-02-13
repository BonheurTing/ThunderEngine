#pragma once
#include "RHIContext.h"
#include "d3dx12.h"

namespace Thunder
{
	class D3D12CommandContext : public RHICommandContext
	{
	public:
		D3D12CommandContext() = delete;
		D3D12CommandContext(const ComPtr<ID3D12CommandAllocator>* inCommandAllocator, ID3D12GraphicsCommandList* inCommandList);
		~D3D12CommandContext();

		//Get
		ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return CommandList; }
		ComPtr<ID3D12Device> GetDevice() const;

		// State cache
		class D3D12StateCache* GetStateCache() const { return StateCache; }

		// Clear
		void ClearDepthStencilView(RHIDepthStencilView* dsv, ERHIClearFlags clearFlags, float depthValue, uint8 stencilValue) override;
		void ClearRenderTargetView(RHIRenderTargetView* rtv, TVector4f clearColor) override;
		void ClearState(TRHIPipelineState* pso) override;
		void ClearUnorderedAccessViewUint(RHIResource* resource, TVector4u clearValue) override;
		void ClearUnorderedAccessViewFloat(RHIResource* resource, TVector4f clearValue) override;

		// Set
		void SetIndexBuffer(RHIIndexBufferRef indexBuffer) override;
		void SetPrimitiveTopology(ERHIPrimitive type) override;
		void SetVertexBuffer(uint32 slot, uint32 numViews, RHIVertexBufferRef vertexBuffer) override;
		void SetBlendFactor(TVector4f const& blendFactor) override;
		void SetRenderTarget(uint32 numRT, TArray<RHIRenderTargetView*> rtvs, RHIDepthStencilView* dsv = nullptr) override;
		void SetScissorRects(TArray<RHIRect*> rects) override;
		void SetViewports(TArray<RHIViewport*> viewports) override;
		void SetPipelineState(TRHIPipelineState* pso) override;
		void BindSRVTable(TShaderRegisterCounts const& shaderRC, const uint64* srvHandles, uint32 count) override;
		void BindCBVs(TShaderRegisterCounts const& shaderRC, const uint64* cbvHandles, uint32 count) override;

		// Copy
		void CopyBufferRegion(RHIResource* dst, uint64 dstOffset, RHIResource* src, uint64 srcOffset, uint64 numBytes) override;
		void CopyTextureRegion(RHIResource* dst, uint32 dstMip, RHIResource* src, uint32 srcMip, const RHITextureRegion* copyRegion) override;
		void CopyResource(RHIResource* dst, RHIResource* src) override;
		void DiscardResource(RHIResource* resource, TArray<RHIRect> const& rects) override;
		void ResolveSubresource(RHIResource* dst, uint32 dstSubId, RHIResource* src, uint32 srcSubId) override;
    	
		// Draw
		void Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) override;
		void DrawIndexedInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation, int32 baseVertexLocation, uint32 startInstanceLocation) override;
		void DrawInstanced(uint32 vertexCountPerInstance, uint32 instanceCount, uint32 startVertexLocation, uint32 startInstanceLocation) override;
		void Execute() override;
    	
		// Misc
		void Reset(uint32 index) override;

		class TD3D12RootSignature* BindRootSignature(TShaderRegisterCounts const& shaderRC) const;

	private:
		ComPtr<ID3D12CommandAllocator> CommandAllocator[MAX_FRAME_LAG];
		ComPtr<ID3D12GraphicsCommandList> CommandList;

		// State cache for online heap management
		class D3D12StateCache* StateCache = nullptr;
	};
}
