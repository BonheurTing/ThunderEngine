#pragma once
#include "RHI.h"
#include "RHIContext.h"
#include "RHIResource.h"
#include "UniformBuffer.h"

namespace Thunder
{
    class IDynamicRHI
    {
    public:
        RHI_API virtual ~IDynamicRHI() = default;

        // Deferred resource deletion. Keeps the resource alive for MAX_FRAME_LAG frames.
        RHI_API void DeferredDeleteResource(TRefCountPtr<RHIResource> resource);
        RHI_API void FlushDeferredDeleteQueue();

        /////// RHI Methods
        RHI_API virtual RHIDeviceRef RHICreateDevice() = 0;

        RHI_API virtual void RHICreateCommandQueue() {}

        RHI_API virtual RHICommandContextRef RHICreateCommandContext() = 0;
        
        RHI_API virtual TRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer) = 0;
        
        RHI_API virtual void RHICreateComputePipelineState() = 0;
        
        RHI_API virtual void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) = 0;
        
        RHI_API virtual void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) = 0;
        
        RHI_API virtual void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) = 0;
        
        RHI_API virtual void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) = 0;
        
        RHI_API virtual void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) = 0;

        RHI_API virtual RHISamplerRef RHICreateSampler(const RHISamplerDescriptor& desc) = 0;

        RHI_API virtual RHIFenceRef RHICreateFence(uint64 initValue, uint32 fenceFlags) = 0;
    
        RHI_API virtual RHIVertexBufferRef RHICreateVertexBuffer(uint32 sizeInBytes, uint32 strideInBytes, EBufferCreateFlags usage, void* resourceData = nullptr) = 0;
    
        RHI_API virtual RHIIndexBufferRef RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, EBufferCreateFlags usage, void* resourceData = nullptr) = 0;
    
        RHI_API virtual RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, EBufferCreateFlags usage, void* resourceData = nullptr) = 0;
        
        RHI_API virtual RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, EBufferCreateFlags usage, void* resourceData = nullptr) = 0;

        RHI_API virtual RHIUniformBufferRef RHICreateUniformBuffer(uint32 size, EUniformBufferFlags usage, const void* Contents) = 0;

        RHI_API virtual void RHIUpdateUniformBuffer(struct IRHICommandRecorder* recorder, RHIUniformBuffer* unformBuffer, const void* Contents) = 0;
    
        RHI_API virtual RHITextureRef RHICreateTexture(const RHIResourceDescriptor& desc, ETextureCreateFlags usage, void* resourceData = nullptr) = 0;

        RHI_API virtual bool RHIUpdateSharedMemoryResource(RHIResource* resource, const void* resourceData, uint32 size, uint8 subresourceId) = 0;

        RHI_API virtual void RHIReleaseResource_RenderThread() = 0;
        RHI_API virtual void RHIReleaseResource_RHIThread() = 0;

        // CPU-GPU synchronization and frame lifecycle
        RHI_API virtual void RHIBeginFrame(uint32 frameIndex) = 0;
        RHI_API virtual void RHISignalFence(uint32 frameIndex) = 0;
        RHI_API virtual void RHIWaitForFrame(uint32 frameIndex) = 0;

        RHI_API void SetViewportResolution(uint32 width, uint32 height) { ViewportResolution.X = width; ViewportResolution.Y = height; }
        RHI_API TVector2u GetViewportResolution() const { return ViewportResolution; }
        RHI_API uint32 GetViewportWidth() const { return ViewportResolution.X; }
        RHI_API uint32 GetViewportHeight() const { return ViewportResolution.Y; }

        // Window presentation
        RHI_API virtual void RHICreateSwapChain(void* hwnd, uint32 width, uint32 height) {}
        RHI_API virtual void RHIPresent() {}

    protected:
        TArray<TRefCountPtr<RHIResource>> DeferredDeleteQueue[MAX_FRAME_LAG] {};
        TVector2u ViewportResolution{ 1920, 1080 };
    };
    
    extern RHI_API IDynamicRHI* GDynamicRHI;
    
    FORCEINLINE RHIDeviceRef RHICreateDevice()
    {
        return GDynamicRHI->RHICreateDevice();
    }

    FORCEINLINE void RHICreateCommandQueue()
    {
        GDynamicRHI->RHICreateCommandQueue();
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
    
    FORCEINLINE RHIVertexBufferRef RHICreateVertexBuffer(uint32 sizeInBytes, uint32 strideInBytes, EBufferCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateVertexBuffer(sizeInBytes, strideInBytes, usage, resourceData);
    }
    
    FORCEINLINE RHIIndexBufferRef RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, EBufferCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateIndexBuffer(width, type, usage, resourceData);
    }
    
    FORCEINLINE RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, EBufferCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateStructuredBuffer(size, usage, resourceData);
    }
    
    FORCEINLINE RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, EBufferCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateConstantBuffer(size, usage, resourceData);
    }

    FORCEINLINE RHIUniformBufferRef RHICreateUniformBuffer(uint32 size, EUniformBufferFlags usage, const void* Contents)
    {
        return GDynamicRHI->RHICreateUniformBuffer(size, usage, Contents);
    }

    FORCEINLINE void RHIUpdateUniformBuffer(IRHICommandRecorder* recorder, RHIUniformBuffer* unformBuffer, const void* Contents)
    {
        return GDynamicRHI->RHIUpdateUniformBuffer(recorder, unformBuffer, Contents);
    }

    FORCEINLINE RHITextureRef RHICreateTexture(const RHIResourceDescriptor& desc, ETextureCreateFlags usage, void* resourceData = nullptr)
    {
        return GDynamicRHI->RHICreateTexture(desc, usage, resourceData);
    }

    FORCEINLINE bool RHIUpdateSharedMemoryResource(RHIResource* resource, const void* resourceData, uint32 size, uint8 subresourceId)
    {
        return GDynamicRHI->RHIUpdateSharedMemoryResource(resource, resourceData, size, subresourceId);
    }

    FORCEINLINE void RHIDeferredDeleteResource(TRefCountPtr<RHIResource> resource)
    {
        GDynamicRHI->DeferredDeleteResource(std::move(resource));
    }

    FORCEINLINE void RHIReleaseResource_RenderThread()
    {
        GDynamicRHI->FlushDeferredDeleteQueue();
    }

    FORCEINLINE void RHIReleaseResource_RHIThread()
    {
        GDynamicRHI->RHIReleaseResource_RHIThread();
    }

    FORCEINLINE void RHIBeginFrame(uint32 frameIndex)
    {
        return GDynamicRHI->RHIBeginFrame(frameIndex);
    }

    FORCEINLINE void RHISignalFence(uint32 frameIndex)
    {
        return GDynamicRHI->RHISignalFence(frameIndex);
    }

    FORCEINLINE void RHIWaitForFrame(uint32 frameIndex)
    {
        return GDynamicRHI->RHIWaitForFrame(frameIndex);
    }

    FORCEINLINE void RHICreateSwapChain(void* hwnd, uint32 width, uint32 height)
    {
        GDynamicRHI->RHICreateSwapChain(hwnd, width, height);
    }

    FORCEINLINE void RHIPresent()
    {
        GDynamicRHI->RHIPresent();
    }

    RHI_API extern std::atomic_uint64_t GCachedMeshDrawCommandIndex;
    FORCEINLINE uint64 RHIAllocateCachedMeshDrawCommandIndex()
    {
        TAssertf(GCachedMeshDrawCommandIndex.load(std::memory_order_acquire) < 0xFFFFFFFFFFFFFFFFull, "Cached mesh-draw index is out-of-range.");
        return GCachedMeshDrawCommandIndex.fetch_add(1, std::memory_order_acq_rel);
    }
}

