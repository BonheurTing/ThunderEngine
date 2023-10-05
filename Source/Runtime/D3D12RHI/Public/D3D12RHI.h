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
    
        RHIRasterizerStateRef RHICreateRasterizerState(const RasterizerStateInitializerRHI& Initializer) override;
    
        RHIDepthStencilStateRef RHICreateDepthStencilState(const DepthStencilStateInitializerRHI& Initializer) override;
    
        RHIBlendStateRef RHICreateBlendState(const BlendStateInitializerRHI& Initializer) override;
    
        RHIInputLayoutRef RHICreateInputLayout(const RHIInputLayoutDescriptor& initializer) override;
    
        RHIVertexDeclarationRef RHICreateVertexDeclaration(const VertexDeclarationInitializerRHI& Elements) override;

       void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) override;
        
        void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
        void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
        void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) override;
        
        void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) override;
    
        RHIVertexBufferRef RHICreateVertexBuffer(const RHIResourceDescriptor& desc) override;
        
        RHIIndexBufferRef RHICreateIndexBuffer(const RHIResourceDescriptor& desc) override;
    
        RHIStructuredBufferRef RHICreateStructuredBuffer(const RHIResourceDescriptor& desc) override;
    
        RHIConstantBufferRef RHICreateConstantBuffer(const RHIResourceDescriptor& desc) override;
    
        RHITexture1DRef RHICreateTexture1D(const RHIResourceDescriptor& desc) override;
    
        RHITexture2DRef RHICreateTexture2D(const RHIResourceDescriptor& desc) override;
    
        RHITexture2DArrayRef RHICreateTexture2DArray(const RHIResourceDescriptor& desc) override;
    
        RHITexture3DRef RHICreateTexture3D(const RHIResourceDescriptor& desc) override;
    
    private:
        ComPtr<ID3D12Device> Device;

        // descriptor heap
        RefCountPtr<TD3D12DescriptorHeap> CommonDescriptorHeap; //cbv srv uav
        RefCountPtr<TD3D12DescriptorHeap> RTVDescriptorHeap;
        RefCountPtr<TD3D12DescriptorHeap> DSVDescriptorHeap;
        RefCountPtr<TD3D12DescriptorHeap> SamplerDescriptorHeap;
    };
}
