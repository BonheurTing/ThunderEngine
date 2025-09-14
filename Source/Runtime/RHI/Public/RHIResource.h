#pragma once

#include "RHI.h"
#include "CoreMinimal.h"
#include "RHI.export.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
    class ShaderCombination;
    
    typedef TRefCountPtr<RHIConstantBufferView> RHIConstantBufferViewRef;
    typedef TRefCountPtr<RHIShaderResourceView> RHIShaderResourceViewRef;
    typedef TRefCountPtr<RHIUnorderedAccessView> RHIUnorderedAccessViewRef;
    typedef TRefCountPtr<RHIRenderTargetView> RHIRenderTargetViewRef;
    typedef TRefCountPtr<RHIDepthStencilView> RHIDepthStencilViewRef;

    
    class RHI_API RHIResource : public RefCountedObject
    {
    public:
        RHIResource(RHIResourceDescriptor const& desc) : Desc(desc) {}
        virtual ~RHIResource() = default;

        _NODISCARD_ virtual void* GetResource() const = 0;
        _NODISCARD_ const RHIResourceDescriptor* GetResourceDescriptor() const { return &Desc; }
        void SetSRV(RHIShaderResourceView* view)
        {
            SRV = view;
        }
        void SetUAV(RHIUnorderedAccessView* view)
        {
            UAV = view;
        }

        virtual void Update() {};

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
        
        void SetCBV(RHIConstantBufferView* view)
        {
            CBV = view;
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
        RHITexture(RHIResourceDescriptor const& desc, ETextureCreateFlags const& flags) : RHIResource(desc), CreateFlags(flags) {}

        void SetRTV(RHIRenderTargetView* view)
        {
            RTV = view;
        }
        void SetDSV(RHIDepthStencilView* view)
        {
            DSV = view;
        }

        void SetBinaryData(BinaryDataRef src)
        {
            Data = src;
        }
        
        _NODISCARD_ virtual RHIRenderTargetViewRef GetRTV() const { return RTV; }
        _NODISCARD_ virtual RHIDepthStencilViewRef GetDSV() const { return DSV; }
    protected:
        RHIRenderTargetViewRef RTV;
        RHIDepthStencilViewRef DSV;

        ETextureCreateFlags CreateFlags;
        BinaryDataRef Data;
    };
    using RHITextureRef = TRefCountPtr<RHITexture>;
    
    
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
    // Pipeline state
    //

    class TRHIPipelineState : public RefCountedObject
    {
    public:
        TRHIPipelineState(ERHIPipelineStateType type) : PipelineStateType(type) {}
        virtual ~TRHIPipelineState() = default;
        _NODISCARD_ virtual void* GetPipelineState() const { return nullptr; }
        _NODISCARD_ ERHIPipelineStateType GetPipelineStateType() const { return PipelineStateType; }
        
    private:
        ERHIPipelineStateType PipelineStateType;
    };
    
    class TRHIGraphicsPipelineState : public TRHIPipelineState
    {
    public:
        TRHIGraphicsPipelineState(const TGraphicsPipelineStateDescriptor& desc) :
            TRHIPipelineState(ERHIPipelineStateType::Graphics), PipelineStateDesc(desc) {}
    private:
        TGraphicsPipelineStateDescriptor PipelineStateDesc;
    };

    class TRHIComputePipelineState : public TRHIPipelineState
    {
    public:
        TRHIComputePipelineState(const TComputePipelineStateDescriptor& desc) :
            TRHIPipelineState(ERHIPipelineStateType::Compute), PipelineStateDesc(desc) {}
    private:
        TComputePipelineStateDescriptor PipelineStateDesc;
    };

    //
    // State blocks npv_
    //
    
    class RHIDevice : public RefCountedObject
    {
    public:
        RHIDevice(){}
    };

    class RHIViewport
    {
    public:
        virtual ~RHIViewport() = default;
        _NODISCARD_ virtual const void* GetViewPort() const = 0;
    };

    ////// REF
    typedef TRefCountPtr<RHIDevice> RHIDeviceRef;
    typedef TRefCountPtr<RHIBlendState> RHIBlendStateRef;
    typedef TRefCountPtr<RHIRasterizerState> RHIRasterizerStateRef;
    typedef TRefCountPtr<RHIDepthStencilState> RHIDepthStencilStateRef;
    typedef TRefCountPtr<RHIVertexDeclarationDescriptor> RHIVertexDeclarationRef;
    typedef TRefCountPtr<TRHIGraphicsPipelineState> RHGraphicsPipelineStateIRef;    
    typedef TRefCountPtr<RHISampler> RHISamplerRef;
    typedef TRefCountPtr<RHIFence> RHIFenceRef;
    typedef TRefCountPtr<RHIVertexBuffer> RHIVertexBufferRef;
    typedef TRefCountPtr<RHIIndexBuffer> RHIIndexBufferRef;
    typedef TRefCountPtr<RHIStructuredBuffer> RHIStructuredBufferRef;
    typedef TRefCountPtr<RHIConstantBuffer> RHIConstantBufferRef;
    typedef TRefCountPtr<RHITexture> RHITextureRef;
}
