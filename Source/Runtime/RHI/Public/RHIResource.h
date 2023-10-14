#pragma once

#include "RHI.h"
#include "RHIContext.h"
#include "CoreMinimal.h"
#include "RHI.export.h"

namespace Thunder
{

    
    typedef RefCountPtr<RHIConstantBufferView> RHIConstantBufferViewRef;
    typedef RefCountPtr<RHIShaderResourceView> RHIShaderResourceViewRef;
    typedef RefCountPtr<RHIUnorderedAccessView> RHIUnorderedAccessViewRef;
    typedef RefCountPtr<RHIRenderTargetView> RHIRenderTargetViewRef;
    typedef RefCountPtr<RHIDepthStencilView> RHIDepthStencilViewRef;

    
    class RHI_API RHIResource
    {
    public:
        RHIResource(RHIResourceDescriptor const& desc) : Desc(desc) {}
        virtual ~RHIResource() = default;

        _NODISCARD_ virtual void* GetResource() const = 0;
        _NODISCARD_ const RHIResourceDescriptor* GetResourceDescriptor() const { return &Desc; }
        void SetSRV(const RHIShaderResourceView& view)
        {
            SRV = MakeRefCount<RHIShaderResourceView>(view);
        }
        void SetUAV(const RHIUnorderedAccessView& view)
        {
            UAV = MakeRefCount<RHIUnorderedAccessView>(view);
        }

        _NODISCARD_ RHIShaderResourceViewRef GetSRV() const { return SRV; }
        _NODISCARD_ RHIUnorderedAccessViewRef GetUAV() const { return UAV; }

    protected:
        RHIResourceDescriptor Desc = {};
        RHIShaderResourceViewRef SRV;
        RHIUnorderedAccessViewRef UAV;
    };
    
    class RHIBuffer : public RHIResource
    {
    public:
        RHIBuffer(RHIResourceDescriptor const& desc) : RHIResource(desc) {}
        
        void SetCBV(const RHIConstantBufferView& view)
        {
            CBV = MakeRefCount<RHIConstantBufferView>(view);
        }
        
        _NODISCARD_ virtual RHIConstantBufferViewRef GetCBV() const { return CBV; }
    protected:
        RHIConstantBufferViewRef CBV;
    };
    
    /**
     * \brief texture
     */
    
    class RHITexture : public RHIResource
    {
    public:
        RHITexture(RHIResourceDescriptor const& desc) : RHIResource(desc) {}

        void SetRTV(const RHIRenderTargetView& view)
        {
            RTV = MakeRefCount<RHIRenderTargetView>(view);
        }
        void SetDSV(const RHIDepthStencilView& view)
        {
            DSV = MakeRefCount<RHIDepthStencilView>(view);
        }
        
        _NODISCARD_ virtual RHIRenderTargetViewRef GetRTV() const { return RTV; }
        _NODISCARD_ virtual RHIDepthStencilViewRef GetDSV() const { return DSV; }
    protected:
        RHIRenderTargetViewRef RTV;
        RHIDepthStencilViewRef DSV;
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

    class RHIViewPort
    {
    public:
        RHIViewPort(){}
    };
    
    
    typedef int RHIVertexShader;
    typedef int RHIPixelShader;
    
    
    
    
    ////// REF
    typedef RefCountPtr<RHIDevice> RHIDeviceRef;
    typedef RefCountPtr<RHICommandContext> RHICommandContextRef;
    typedef RefCountPtr<RHIRasterizerState> RHIRasterizerStateRef;
    typedef RefCountPtr<RHIDepthStencilState> RHIDepthStencilStateRef;
    typedef RefCountPtr<RHIBlendState> RHIBlendStateRef;
    typedef RefCountPtr<RHIInputLayout> RHIInputLayoutRef;
    typedef RefCountPtr<RHIVertexDeclaration> RHIVertexDeclarationRef;
    typedef RefCountPtr<RHIVertexShader> RHIVertexShaderRef;
    typedef RefCountPtr<RHIPixelShader> RHIPixelShaderRef;
    typedef RefCountPtr<RHISampler> RHISamplerRef;
    typedef RefCountPtr<RHIFence> RHIFenceRef;
    typedef RefCountPtr<RHIVertexBuffer> RHIVertexBufferRef;
    typedef RefCountPtr<RHIIndexBuffer> RHIIndexBufferRef;
    typedef RefCountPtr<RHIStructuredBuffer> RHIStructuredBufferRef;
    typedef RefCountPtr<RHIConstantBuffer> RHIConstantBufferRef;
    typedef RefCountPtr<RHITexture1D> RHITexture1DRef;
    typedef RefCountPtr<RHITexture2D> RHITexture2DRef;
    typedef RefCountPtr<RHITexture2DArray> RHITexture2DArrayRef;
    typedef RefCountPtr<RHITexture3D> RHITexture3DRef;

    
}
