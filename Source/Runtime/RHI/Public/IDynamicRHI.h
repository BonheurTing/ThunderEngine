#pragma once
#include "RHI.h"
#include "RHIContext.h"
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
        
        virtual TRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer) = 0;
        
        virtual void RHICreateComputePipelineState() = 0;
        
        virtual void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) = 0;
        
        virtual void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) = 0;
        
        virtual void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) = 0;
        
        virtual void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) = 0;
        
        virtual void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) = 0;

        virtual RHISamplerRef RHICreateSampler(const RHISamplerDescriptor& desc) = 0;

        virtual RHIFenceRef RHICreateFence(uint64 initValue, uint32 fenceFlags) = 0;
    
        virtual RHIVertexBufferRef RHICreateVertexBuffer(uint32 sizeInBytes, uint32 StrideInBytes, ETextureCreateFlags usage, void* resourceData = nullptr) = 0;
    
        virtual RHIIndexBufferRef RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, ETextureCreateFlags usage, void* resourceData = nullptr) = 0;
    
        virtual RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, ETextureCreateFlags usage, void* resourceData = nullptr) = 0;
        
        virtual RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, ETextureCreateFlags usage, void* resourceData = nullptr) = 0;
    
        virtual RHITextureRef RHICreateTexture(const RHIResourceDescriptor& desc, ETextureCreateFlags usage, void* resourceData = nullptr) = 0;

        virtual bool RHIUpdateSharedMemoryResource(RHIResource* resource, void* resourceData, uint32 size, uint8 subresourceId) = 0;

        virtual void RHIReleaseResource() = 0;
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

    FORCEINLINE TRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer)
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
    
    FORCEINLINE RHIVertexBufferRef RHICreateVertexBuffer(uint32 sizeInBytes, uint32 StrideInBytes, ETextureCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateVertexBuffer(sizeInBytes, StrideInBytes, usage, resourceData);
    }
    
    FORCEINLINE RHIIndexBufferRef RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, ETextureCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateIndexBuffer(width, type, usage, resourceData);
    }
    
    FORCEINLINE RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, ETextureCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateStructuredBuffer(size, usage, resourceData);
    }
    
    FORCEINLINE RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, ETextureCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateConstantBuffer(size, usage, resourceData);
    }

    FORCEINLINE RHITextureRef RHICreateTexture(const RHIResourceDescriptor& desc, ETextureCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateTexture(desc, usage, resourceData);
    }

    FORCEINLINE bool RHIUpdateSharedMemoryResource(RHIResource* resource, void* resourceData, uint32 size, uint8 subresourceId)
    {
        return GDynamicRHI->RHIUpdateSharedMemoryResource(resource, resourceData, size, subresourceId);
    }

    FORCEINLINE void RHIReleaseResource()
    {
        return GDynamicRHI->RHIReleaseResource();
    }
}

