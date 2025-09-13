#pragma once
#include "IDynamicRHI.h"
#include "d3d11.h"

namespace Thunder
{
	class D3D11RHI_API D3D11DynamicRHI : public IDynamicRHI
	{
	public:
		D3D11DynamicRHI() {}
		virtual ~D3D11DynamicRHI() = default;
	    
		/////// RHI Methods
		RHIDeviceRef RHICreateDevice() override;

		RHICommandContextRef RHICreateCommandContext() override;
		
		TRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer) override;
        
		void RHICreateComputePipelineState() override {}

		void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) override;
        
		void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
		void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
		void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) override;
        
		void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) override;
		
        RHISamplerRef RHICreateSampler(const RHISamplerDescriptor& desc) override;
		
        RHIFenceRef RHICreateFence(uint64 initValue, uint32 fenceFlags) override;

		RHIVertexBufferRef RHICreateVertexBuffer(uint32 sizeInBytes, uint32 StrideInBytes, EResourceUsageFlags usage, void *resourceData = nullptr) override;
        
		RHIIndexBufferRef RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
		RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
		RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
		RHITextureRef RHICreateTexture(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr) override;

		void* RHIMapTexture2D(RHITexture* Texture, uint32 MipIndex, uint32 LockMode, uint32& DestStride) override;

		void RHIUnmapTexture2D(RHITexture* Texture, uint32 MipIndex) override;

		void RHIUpdateTexture(RHITexture* Texture) override;

		bool RHIUpdateSharedMemoryResource(RHIResource* resource, void* resourceData, uint32 size, uint8 subresourceId) override;

	private:
		ComPtr<ID3D11Device> Device;
	};
}