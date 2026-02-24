#pragma once
#include "IDynamicRHI.h"
#include "d3d11.h"

namespace Thunder
{
	class D3D11DynamicRHI : public IDynamicRHI
	{
	public:
		D3D11RHI_API D3D11DynamicRHI() {}
		D3D11RHI_API virtual ~D3D11DynamicRHI() = default;
	    
		/////// RHI Methods
		D3D11RHI_API RHIDeviceRef RHICreateDevice() override;

		D3D11RHI_API ID3D11Device* GetDevice() const { return Device.Get(); }

		D3D11RHI_API RHICommandContextRef RHICreateCommandContext() override;
		
		D3D11RHI_API TRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer) override;
        
		D3D11RHI_API void RHICreateComputePipelineState() override {}

		D3D11RHI_API void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) override;
        
		D3D11RHI_API void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
		D3D11RHI_API void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
		D3D11RHI_API void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) override;
        
		D3D11RHI_API void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) override;
		
        D3D11RHI_API RHISamplerRef RHICreateSampler(const RHISamplerDescriptor& desc) override;
		
        D3D11RHI_API RHIFenceRef RHICreateFence(uint64 initValue, uint32 fenceFlags) override;

		D3D11RHI_API RHIVertexBufferRef RHICreateVertexBuffer(uint32 sizeInBytes, uint32 strideInBytes, EBufferCreateFlags usage, void *resourceData = nullptr) override;
        
		D3D11RHI_API RHIIndexBufferRef RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, EBufferCreateFlags usage, void *resourceData = nullptr) override;
    
		D3D11RHI_API RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, EBufferCreateFlags usage, void *resourceData = nullptr) override;
    
		D3D11RHI_API RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, EBufferCreateFlags usage, void *resourceData = nullptr) override;
		
        D3D11RHI_API RHIUniformBufferRef RHICreateUniformBuffer(uint32 size, EUniformBufferFlags usage, const void* Contents) override { return nullptr; }

		D3D11RHI_API void RHIUpdateUniformBuffer(IRHICommandRecorder* recorder, RHIUniformBuffer* unformBuffer, const void* Contents) override { }
    
		D3D11RHI_API RHITextureRef RHICreateTexture(const RHIResourceDescriptor& desc, ETextureCreateFlags usage, void *resourceData = nullptr) override;

		D3D11RHI_API bool RHIUpdateSharedMemoryResource(RHIResource* resource, const void* resourceData, uint32 size, uint8 subresourceId) override;

		D3D11RHI_API void RHIReleaseResource_RenderThread() override {}
		D3D11RHI_API void RHIReleaseResource_RHIThread() override {}

		D3D11RHI_API void RHIBeginFrame(uint32 frameIndex) override {}

		D3D11RHI_API void RHISignalFence(uint32 frameIndex) override {}

		D3D11RHI_API void RHIWaitForFrame(uint32 frameIndex) override {}

	private:
		ComPtr<ID3D11Device> Device;
	};
}