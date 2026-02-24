#pragma once

#include "IDynamicRHI.h"
#include "d3d12.h"
#include "D3D12DescriptorHeap.h"
#include "Concurrent/Lock.h"
#include "HAL/Event.h"
#include <dxgi1_4.h>

namespace Thunder
{
    class D3D12DynamicRHI : public IDynamicRHI
    {
    public:
        D3D12RHI_API D3D12DynamicRHI();
        D3D12RHI_API ~D3D12DynamicRHI();

        D3D12RHI_API RHIDeviceRef RHICreateDevice() override;

        D3D12RHI_API void RHICreateCommandQueue() override; 

        D3D12RHI_API RHICommandContextRef RHICreateCommandContext() override;

        D3D12RHI_API TRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer) override;

        D3D12RHI_API void RHICreateComputePipelineState() override;

        D3D12RHI_API void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) override;

        D3D12RHI_API void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) override;

        D3D12RHI_API void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) override;

        D3D12RHI_API void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) override;

        D3D12RHI_API void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) override;

        D3D12RHI_API RHISamplerRef RHICreateSampler(const RHISamplerDescriptor& desc) override;

        D3D12RHI_API RHIFenceRef RHICreateFence(uint64 initValue, uint32 fenceFlags) override;

        D3D12RHI_API RHIVertexBufferRef RHICreateVertexBuffer(uint32 sizeInBytes, uint32 strideInBytes, EBufferCreateFlags usage, void *resourceData = nullptr) override;

        D3D12RHI_API RHIIndexBufferRef RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, EBufferCreateFlags usage, void *resourceData = nullptr) override;

        D3D12RHI_API RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, EBufferCreateFlags usage, void *resourceData = nullptr) override;

        D3D12RHI_API RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, EBufferCreateFlags usage, void *resourceData = nullptr) override;

        D3D12RHI_API RHIUniformBufferRef RHICreateUniformBuffer(uint32 size, EUniformBufferFlags usage, const void* Contents) override;

        D3D12RHI_API void RHIUpdateUniformBuffer(IRHICommandRecorder* recorder, RHIUniformBuffer* unformBuffer, const void* Contents) override;

        D3D12RHI_API RHITextureRef RHICreateTexture(const RHIResourceDescriptor& desc, ETextureCreateFlags usage, void *resourceData = nullptr) override;

        D3D12RHI_API bool RHIUpdateSharedMemoryResource(RHIResource* resource, const void* resourceData, uint32 size, uint8 subresourceId) override;

        D3D12RHI_API void RHIReleaseResource_RenderThread() override;
        D3D12RHI_API void RHIReleaseResource_RHIThread() override;

        FORCEINLINE ComPtr<ID3D12Device> GetDevice() const { return Device; }
        FORCEINLINE ID3D12CommandQueue* RHIGetD3DCommandQueue() const { return CommandQueue.Get(); }

        D3D12RHI_API void AddReleaseObject(ComPtr<ID3D12Object> object);

        D3D12RHI_API void RHIBeginFrame(uint32 frameIndex) override;
        D3D12RHI_API void RHISignalFence(uint32 frameIndex) override;
        D3D12RHI_API void RHIWaitForFrame(uint32 frameIndex) override;

        D3D12RHI_API void RHICreateSwapChain(void* hwnd, uint32 width, uint32 height) override;
        D3D12RHI_API void RHIPresent() override;

        FORCEINLINE ID3D12Resource*             GetCurrentBackBuffer() const { return SwapChainBuffers[CurrentBackBufferIndex].Get(); }
        FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const { return BackBufferRTVs[CurrentBackBufferIndex].Handle; }

        // Online descriptor manager for global heap (GPU-visible, transient)
        FORCEINLINE class D3D12OnlineDescriptorManager* GetOnlineDescriptorManager() const { return OnlineDescriptorManager; }

        // Offline descriptor managers (CPU-side, persistent)
        D3D12RHI_API class D3D12OfflineDescriptorManager* GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

        // Fence utilities
        FORCEINLINE uint64 GetCurrentFenceValue() const { return CurrentFenceValue; }
        D3D12RHI_API bool IsFenceComplete(uint64 fenceValue) const;

        // Null descriptors for binding empty slots
        FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetNullSRV() const { return NullSRV; }
        FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetNullUAV() const { return NullUAV; }
        FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetNullCBV() const { return NullCBV; }

    private:
        D3D12RHI_API void InitializeOnlineDescriptorManager();
        D3D12RHI_API void InitializeOfflineDescriptorManagers();
        D3D12RHI_API void InitializeNullDescriptors();
        D3D12RHI_API void ResizeBackBuffer_RHIThread();

    private:
        ComPtr<ID3D12Device> Device;

        // command queue
        ComPtr<ID3D12CommandQueue> CommandQueue;

        // fence synchronization
        ComPtr<ID3D12Fence> Fence;
        IEvent* FenceEvent;
        uint64 ExpectedFenceValues[MAX_FRAME_LAG] = {0};
        uint64 CurrentFenceValue = 0;

        // Offline descriptor managers (CPU-side, for creating descriptors)
        class D3D12OfflineDescriptorManager* OfflineDescriptorManagers[4] = { nullptr };

        // Online descriptor manager for global heap (GPU-visible, for runtime binding)
        class D3D12OnlineDescriptorManager* OnlineDescriptorManager = nullptr;

        // Uniform buffer allocator
        class D3D12PersistentUploadHeapAllocator* UploadHeapAllocator;

        class D3D12TransientUploadHeapAllocator* TransientUploadAllocator = nullptr;

        // Null descriptors for binding empty slots
        D3D12_CPU_DESCRIPTOR_HANDLE NullSRV = {};
        D3D12_CPU_DESCRIPTOR_HANDLE NullUAV = {};
        D3D12_CPU_DESCRIPTOR_HANDLE NullCBV = {};

        // SwapChain
        ComPtr<IDXGISwapChain3>  SwapChain;
        ComPtr<ID3D12Resource>   SwapChainBuffers[2];
        uint32                   CurrentBackBufferIndex = 0;
        D3D12OfflineDescriptor   BackBufferRTVs[2];
        TVector2u CachedSwapChainResolution = { 0, 0 };
        HWND NativeHwnd = nullptr;

        // Release queue
        static constexpr uint32 ReleaseQueueLength = MAX_FRAME_LAG + 1; // +1 allows render thread to release rhi at any time.
        TArray<ComPtr<ID3D12Object>> ReleaseQueue[ReleaseQueueLength] {};
    };
}
