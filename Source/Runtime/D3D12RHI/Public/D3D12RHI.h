#pragma once

#include "IDynamicRHI.h"
#include "d3d12.h"
#include "D3D12DescriptorHeap.h"

namespace Thunder
{
    //class TD3D12DescriptorHeap;
    
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
    
        RHIVertexBufferRef RHICreateVertexBuffer(uint32 sizeInBytes, uint32 StrideInBytes, ETextureCreateFlags usage, void *resourceData = nullptr) override;
        
        RHIIndexBufferRef RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, ETextureCreateFlags usage, void *resourceData = nullptr) override;
    
        RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, ETextureCreateFlags usage, void *resourceData = nullptr) override;
    
        RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, ETextureCreateFlags usage, void *resourceData = nullptr) override;
    
        RHITextureRef RHICreateTexture(const RHIResourceDescriptor& desc, ETextureCreateFlags usage, void *resourceData = nullptr) override;

        bool RHIUpdateSharedMemoryResource(RHIResource* resource, void* resourceData, uint32 size, uint8 subresourceId) override;

        void RHIReleaseResource() override;

        /// dx12 only
        
        ComPtr<ID3D12Device> GetDevice() const { return Device; }

        void AddReleaseObject(ComPtr<ID3D12Object> object);
    
    private:
        ComPtr<ID3D12Device> Device;

        // descriptor heap
        TRefCountPtr<TD3D12DescriptorHeap> CommonDescriptorHeap; //cbv srv uav
        TRefCountPtr<TD3D12DescriptorHeap> RTVDescriptorHeap;
        TRefCountPtr<TD3D12DescriptorHeap> DSVDescriptorHeap;
        TRefCountPtr<TD3D12DescriptorHeap> SamplerDescriptorHeap;

        //
        TArray<ComPtr<ID3D12Object>> GReleaseQueue[MAX_FRAME_LAG] {};
    };
}
