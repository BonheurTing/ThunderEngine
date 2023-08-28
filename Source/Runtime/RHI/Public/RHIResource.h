#pragma once

#include "RHI.h"
#include "CoreMinimal.h"
#include "RHI.export.h"

namespace Thunder
{
    class RHI_API RHIResource
    {
    public:
        RHIResource(RHIResourceDescriptor const& desc) : Desc(desc) {}
        virtual void* GetShaderResourceView() { return nullptr; }
        virtual void* GetUnorderedAccessView() { return nullptr; }
        virtual void* GetRenderTargetView() { return nullptr; }
        virtual void* GetDepthStencilView() { return nullptr; }
        virtual void* GetResource() { return nullptr; }
    protected:
        RHIResourceDescriptor Desc = {};
    };
    
    
    class RHIBuffer : public RHIResource
    {
    public:
        RHIBuffer(RHIResourceDescriptor const& desc) : RHIResource(desc) {}
        
    };
    
    /**
     * \brief texture
     */
    
    class RHITexture : public RHIResource
    {
    public:
        RHITexture(RHIResourceDescriptor const& desc) : RHIResource(desc) {}
    };
    
    class RHITexture1D : public RHITexture
    {
    public:
        RHITexture1D(RHIResourceDescriptor const& desc) : RHITexture(desc) {}
    };
    
    class RHITexture2D : public RHITexture
    {
    public:
        RHITexture2D(RHIResourceDescriptor const& desc) : RHITexture(desc) {}
    };
    
    class RHITexture2DArray : public RHITexture
    {
    public:
        RHITexture2DArray(RHIResourceDescriptor const& desc) : RHITexture(desc) {}
    };
    
    class RHITexture3D : public RHITexture
    {
    public:
        RHITexture3D(RHIResourceDescriptor const& desc) : RHITexture(desc) {}
    };
    
    /**
     * \brief buffer
     */
     class RHIVertexBuffer : public RHIBuffer
    {
    public:
        RHIVertexBuffer(RHIResourceDescriptor const& desc) : RHIBuffer(desc) {}
    };
    
    class RHIIndexBuffer : public RHIBuffer
    {
    public:
        RHIIndexBuffer(RHIResourceDescriptor const& desc) : RHIBuffer(desc) {}
    };
    
    class RHIStructuredBuffer : public RHIBuffer
    {
    public:
        RHIStructuredBuffer(RHIResourceDescriptor const& desc) : RHIBuffer(desc) {}
    };
    
    class RHIConstantBuffer : public RHIBuffer
    {
    public:
        RHIConstantBuffer(RHIResourceDescriptor const& desc) : RHIBuffer(desc) {}
    };
    
    //
    // Shader bindings
    //
    
    class RHIInputLayout : public RHIResource
    {
    public:
        virtual bool GetInitializer(class InputLayoutInitializerRHI& init) { return false; }
    };
    
    class RHIVertexDeclaration : public RHIResource
    {
    public:
        virtual bool GetInitializer(class VertexDeclarationInitializerRHI& init) { return false; }
    };
    
    class RHIRootSignature : public RHIResource
    {
        
    };
    
    
    //
    // State blocks npv_
    //
    
    class RHIRasterizerState
    {
    public:
        RHIRasterizerState(){}
        virtual bool GetInitializer(struct RasterizerStateInitializerRHI& init) { return false; }
    };
    
    class RHIDepthStencilState
    {
    public:
        RHIDepthStencilState(){}
        virtual bool GetInitializer(struct DepthStencilStateInitializerRHI& init) { return false; }
    };
    
    class RHIBlendState
    {
    public:
        RHIBlendState(){}
        virtual bool GetInitializer(class BlendStateInitializerRHI& init) { return false; }
    };
    
    class RHIDevice
    {
    public:
        RHIDevice(){}
        
    };
    
    
    typedef int RHIVertexShader;
    typedef int RHIPixelShader;
    
    
    
    
    ////// REF
    typedef RefCountPtr<RHIDevice> RHIDeviceRef;
    typedef RefCountPtr<RHIRasterizerState> RHIRasterizerStateRef;
    typedef RefCountPtr<RHIDepthStencilState> RHIDepthStencilStateRef;
    typedef RefCountPtr<RHIBlendState> RHIBlendStateRef;
    typedef RefCountPtr<RHIInputLayout> RHIInputLayoutRef;
    typedef RefCountPtr<RHIVertexDeclaration> RHIVertexDeclarationRef;
    typedef RefCountPtr<RHIVertexShader> RHIVertexShaderRef;
    typedef RefCountPtr<RHIPixelShader> RHIPixelShaderRef;
    typedef RefCountPtr<RHIVertexBuffer> RHIVertexBufferRef;
    typedef RefCountPtr<RHIIndexBuffer> RHIIndexBufferRef;
    typedef RefCountPtr<RHIStructuredBuffer> RHIStructuredBufferRef;
    typedef RefCountPtr<RHIConstantBuffer> RHIConstantBufferRef;
    typedef RefCountPtr<RHITexture1D> RHITexture1DRef;
    typedef RefCountPtr<RHITexture2D> RHITexture2DRef;
    typedef RefCountPtr<RHITexture2DArray> RHITexture2DArrayRef;
    typedef RefCountPtr<RHITexture3D> RHITexture3DRef;
}
