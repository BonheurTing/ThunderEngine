#pragma once
#include "RHI.h"
#include "RHIResource.h"

namespace Thunder
{
    class RHI_API IDynamicRHI
    {
    public:
        virtual ~IDynamicRHI() {}
        
    /////// RHI Methods
        virtual RHIDeviceRef RHICreateDevice();
        
        virtual RHIRasterizerStateRef RHICreateRasterizerState(const RasterizerStateInitializerRHI& initializer) { return nullptr; }
    
        virtual RHIDepthStencilStateRef RHICreateDepthStencilState(const DepthStencilStateInitializerRHI& initializer) { return nullptr; }
    
        virtual RHIBlendStateRef RHICreateBlendState(const BlendStateInitializerRHI& initializer) { return nullptr; }
    
        virtual RHIInputLayoutRef RHICreateInputLayout(const RHIInputLayoutDescriptor& initializer) { return nullptr; }
    
        virtual RHIVertexDeclarationRef RHICreateVertexDeclaration(const VertexDeclarationInitializerRHI& elements) { return nullptr; }
    
        virtual RHIPixelShaderRef RHICreatePixelShader() { return nullptr; }
    
        virtual RHIVertexShaderRef RHICreateVertexShader() { return nullptr; }
    
        virtual RHIVertexBufferRef RHICreateVertexBuffer(const RHIResourceDescriptor& desc) { return nullptr; }
    
        virtual RHIIndexBufferRef RHICreateIndexBuffer(const RHIResourceDescriptor& desc) { return nullptr; }
    
        virtual RHIStructuredBufferRef RHICreateStructuredBuffer(const RHIResourceDescriptor& desc) { return nullptr; }
        
        virtual RHIConstantBufferRef RHICreateConstantBuffer(const RHIResourceDescriptor& desc) { return nullptr; }
    
        virtual RHITexture1DRef RHICreateTexture1D(const RHIResourceDescriptor& desc) { return nullptr; }
    
        virtual RHITexture2DRef RHICreateTexture2D(const RHIResourceDescriptor& desc) { return nullptr; }
    
        virtual RHITexture2DArrayRef RHICreateTexture2DArray(const RHIResourceDescriptor& desc) { return nullptr; }
    
        virtual RHITexture3DRef RHICreateTexture3D(const RHIResourceDescriptor& desc) { return nullptr; }
    
    };
    
    extern RHI_API IDynamicRHI* GDynamicRHI;
    
    FORCEINLINE RHIDeviceRef RHICreateDevice()
    {
        return GDynamicRHI->RHICreateDevice();
    }
    
    FORCEINLINE RHIInputLayoutRef RHICreateInputLayout(const RHIInputLayoutDescriptor& initializer)
    {
        return GDynamicRHI->RHICreateInputLayout(initializer);
    }
    
    FORCEINLINE RHIVertexBufferRef RHICreateVertexBuffer(const RHIResourceDescriptor& desc)
    {
        return GDynamicRHI->RHICreateVertexBuffer(desc);
    }
    
    FORCEINLINE RHIIndexBufferRef RHICreateIndexBuffer(const RHIResourceDescriptor& desc)
    {
        return GDynamicRHI->RHICreateIndexBuffer(desc);
    }
    
    FORCEINLINE RHIStructuredBufferRef RHICreateStructuredBuffer(const RHIResourceDescriptor& desc)
    {
        return GDynamicRHI->RHICreateStructuredBuffer(desc);
    }
    
    FORCEINLINE RHIConstantBufferRef RHICreateConstantBuffer(const RHIResourceDescriptor& desc)
    {
        return GDynamicRHI->RHICreateConstantBuffer(desc);
    }
    
    FORCEINLINE RHITexture1DRef RHICreateTexture1D(const RHIResourceDescriptor& desc)
    {
        return GDynamicRHI->RHICreateTexture1D(desc);
    }
    
    FORCEINLINE RHITexture2DRef RHICreateTexture2D(const RHIResourceDescriptor& desc)
    {
        return GDynamicRHI->RHICreateTexture2D(desc);
    }
    FORCEINLINE RHITexture2DArrayRef RHICreateTexture2DArray(const RHIResourceDescriptor& desc)
    {
        return GDynamicRHI->RHICreateTexture2DArray(desc);
    }
    FORCEINLINE RHITexture3DRef RHICreateTexture3D(const RHIResourceDescriptor& desc)
    {
        return GDynamicRHI->RHICreateTexture3D(desc);
    }
}

