#pragma once

#include "IDynamicRHI.h"
#include "d3d12.h"

namespace Thunder
{
    class TD3D12DescriptorHeap;
    
    class D3D12RHI_API D3D12DynamicRHI : public IDynamicRHI
    {
    public:
        D3D12DynamicRHI();
        
    /////// RHI Methods
        RHIDeviceRef RHICreateDevice() override;

        RHICommandContextRef RHICreateCommandContext() override;
        
        TRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer) override;
        
        void RHICreateComputePipelineState() override;

       void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) override;
        
        void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
        void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
        void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) override;
        
        void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) override;

        RHISamplerRef RHICreateSampler(const RHISamplerDescriptor& desc) override;

        RHIFenceRef RHICreateFence(uint64 initValue, uint32 fenceFlags) override;
    
        RHIVertexBufferRef RHICreateVertexBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) override;
        
        RHIIndexBufferRef RHICreateIndexBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
        RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
        RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
        RHITexture1DRef RHICreateTexture1D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
        RHITexture2DRef RHICreateTexture2D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
        RHITexture2DArrayRef RHICreateTexture2DArray(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
        RHITexture3DRef RHICreateTexture3D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr) override;
    
    private:
        ComPtr<ID3D12Device> Device;

        // descriptor heap
        RefCountPtr<TD3D12DescriptorHeap> CommonDescriptorHeap; //cbv srv uav
        RefCountPtr<TD3D12DescriptorHeap> RTVDescriptorHeap;
        RefCountPtr<TD3D12DescriptorHeap> DSVDescriptorHeap;
        RefCountPtr<TD3D12DescriptorHeap> SamplerDescriptorHeap;
    };
}
