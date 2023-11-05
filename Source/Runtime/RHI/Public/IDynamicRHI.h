#pragma once
#include "RHI.h"
#include "RHIResource.h"

namespace Thunder
{
    class RHI_API IDynamicRHI
    {
    public:
        virtual ~IDynamicRHI() = default;

        /////// RHI Methods
        virtual RHIDeviceRef RHICreateDevice() = 0;

        virtual RHICommandContextRef RHICreateCommandContext() = 0;
        
        virtual RHIVertexDeclarationRef RHICreateVertexDeclaration(const Array<RHIVertexElement>& InElements) = 0;
        
        /*
        virtual RHIBlendStateRef RHICreateBlendState(const BlendStateInitializerRHI& initializer) { return nullptr; }
        
        virtual RHIRasterizerStateRef RHICreateRasterizerState(const RasterizerStateInitializerRHI& initializer) { return nullptr; }
    
        virtual RHIDepthStencilStateRef RHICreateDepthStencilState(const DepthStencilStateInitializerRHI& initializer) { return nullptr; }*/
        
        virtual RHGraphicsPipelineStateIRef RHICreateGraphicsPipelineState(TGraphicsPipelineStateInitializer& initializer) = 0;
        
        virtual void RHICreateComputePipelineState() = 0;

        virtual void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) = 0;
        
        virtual void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) = 0;
        
        virtual void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) = 0;
        
        virtual void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) = 0;
        
        virtual void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) = 0;

        virtual RHISamplerRef RHICreateSampler(const RHISamplerDescriptor& desc) = 0;

        virtual RHIFenceRef RHICreateFence(uint64 initValue, uint32 fenceFlags) = 0;
    
        virtual RHIVertexBufferRef RHICreateVertexBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) = 0;
    
        virtual RHIIndexBufferRef RHICreateIndexBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) = 0;
    
        virtual RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) = 0;
        
        virtual RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr) = 0;
    
        virtual RHITexture1DRef RHICreateTexture1D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr) = 0;
    
        virtual RHITexture2DRef RHICreateTexture2D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr) = 0;
    
        virtual RHITexture2DArrayRef RHICreateTexture2DArray(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr) = 0;
    
        virtual RHITexture3DRef RHICreateTexture3D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr) = 0;
    };
    
    extern RHI_API IDynamicRHI* GDynamicRHI;
    
    FORCEINLINE RHIDeviceRef RHICreateDevice()
    {
        return GDynamicRHI->RHICreateDevice();
    }

    FORCEINLINE RHICommandContextRef RHICreateCommandContext()
    {
        return GDynamicRHI->RHICreateCommandContext();
    }

    FORCEINLINE RHIVertexDeclarationRef RHICreateVertexDeclaration(const Array<RHIVertexElement>& InElements)
    {
        return GDynamicRHI->RHICreateVertexDeclaration(InElements);
    }

    FORCEINLINE RHGraphicsPipelineStateIRef RHICreateGraphicsPipelineState(TGraphicsPipelineStateInitializer& initializer)
    {
        return GDynamicRHI->RHICreateGraphicsPipelineState(initializer);
    }
        
    FORCEINLINE void RHICreateComputePipelineState()
    {
        return GDynamicRHI->RHICreateComputePipelineState();
    }

    FORCEINLINE void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize)
    {
        return GDynamicRHI->RHICreateConstantBufferView(resource, bufferSize);
    }
    
    FORCEINLINE void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc)
    {
        GDynamicRHI->RHICreateShaderResourceView(resource, desc);
    }
    
    FORCEINLINE void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc)
    {
        return GDynamicRHI->RHICreateUnorderedAccessView(resource, desc);
    }
    
    FORCEINLINE void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc)
    {
        return GDynamicRHI->RHICreateRenderTargetView(resource, desc);
    }
    
    FORCEINLINE void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc)
    {
        return GDynamicRHI->RHICreateDepthStencilView(resource, desc);
    }

    FORCEINLINE RHISamplerRef RHICreateSampler(const RHISamplerDescriptor& desc)
    {
        return GDynamicRHI->RHICreateSampler(desc);
    }

    FORCEINLINE RHIFenceRef RHICreateFence(uint64 initValue, uint32 fenceFlags)
    {
        return GDynamicRHI->RHICreateFence(initValue, fenceFlags);
    }
    
    FORCEINLINE RHIVertexBufferRef RHICreateVertexBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateVertexBuffer(size, usage, resourceData);
    }
    
    FORCEINLINE RHIIndexBufferRef RHICreateIndexBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateIndexBuffer(size, usage, resourceData);
    }
    
    FORCEINLINE RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateStructuredBuffer(size, usage, resourceData);
    }
    
    FORCEINLINE RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateConstantBuffer(size, usage, resourceData);
    }
    
    FORCEINLINE RHITexture1DRef RHICreateTexture1D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateTexture1D(desc, usage, resourceData);
    }
    
    FORCEINLINE RHITexture2DRef RHICreateTexture2D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateTexture2D(desc, usage, resourceData);
    }
    FORCEINLINE RHITexture2DArrayRef RHICreateTexture2DArray(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateTexture2DArray(desc, usage, resourceData);
    }
    FORCEINLINE RHITexture3DRef RHICreateTexture3D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateTexture3D(desc, usage, resourceData);
    }
}

