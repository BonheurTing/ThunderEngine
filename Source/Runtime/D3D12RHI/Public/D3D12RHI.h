#pragma once

#include "IDynamicRHI.h"
#include "d3d12.h"

namespace Thunder
{
    class D3D12RHI_API D3D12DynamicRHI : public IDynamicRHI
    {
    public:
        D3D12DynamicRHI();
        virtual ~D3D12DynamicRHI() = default;
        
    /////// RHI Methods
        RHIDeviceRef RHICreateDevice() override;
    
        RHIRasterizerStateRef RHICreateRasterizerState(const RasterizerStateInitializerRHI& Initializer) override;
    
        RHIDepthStencilStateRef RHICreateDepthStencilState(const DepthStencilStateInitializerRHI& Initializer) override;
    
        RHIBlendStateRef RHICreateBlendState(const BlendStateInitializerRHI& Initializer) override;
    
        RHIInputLayoutRef RHICreateInputLayout(const RHIInputLayoutDescriptor& initializer) override;
    
        RHIVertexDeclarationRef RHICreateVertexDeclaration(const VertexDeclarationInitializerRHI& Elements) override;
    
        RHIPixelShaderRef RHICreatePixelShader() override;
    
        RHIVertexShaderRef RHICreateVertexShader() override;
    
        RHIVertexBufferRef RHICreateVertexBuffer(const RHIResourceDescriptor& desc) override;
        
        RHIIndexBufferRef RHICreateIndexBuffer(const RHIResourceDescriptor& desc) override;
    
        RHIStructuredBufferRef RHICreateStructuredBuffer(const RHIResourceDescriptor& desc) override;
    
        RHIConstantBufferRef RHICreateConstantBuffer(const RHIResourceDescriptor& desc) override;
    
        RHITexture1DRef RHICreateTexture1D(const RHIResourceDescriptor& desc) override;
    
        RHITexture2DRef RHICreateTexture2D(const RHIResourceDescriptor& desc) override;
    
        RHITexture2DArrayRef RHICreateTexture2DArray(const RHIResourceDescriptor& desc) override;
    
        RHITexture3DRef RHICreateTexture3D(const RHIResourceDescriptor& desc) override;


    private:
    /////// DX12 Methods
        void CreateRootSignature() {}
        
    private:
        ComPtr<ID3D12Device> Device;
    };
}
