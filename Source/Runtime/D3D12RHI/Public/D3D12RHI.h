#pragma once

#include "IDynamicRHI.h"
#include "d3d12.h"
#include "D3D12DescriptorHeap.h"
#include "HAL/Event.h"

namespace Thunder
{
    class D3D12RHI_API D3D12DynamicRHI : public IDynamicRHI
    {
    public:
        D3D12DynamicRHI();
        ~D3D12DynamicRHI();
        
    /////// RHI Methods
        RHIDeviceRef RHICreateDevice() override;

        void RHICreateCommandQueue() override; 

        RHICommandContextRef RHICreateCommandContext() override;
        
        TRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer) override;
        
        void RHICreateComputePipelineState() override;

       void RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize) override;
        
        void RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
        void RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc) override;
        
        void RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc) override;
        
        void RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc) override;

        RHISamplerRef RHICreateSampler(const RHISamplerDescriptor& desc) override;

        RHIFenceRef RHICreateFence(uint64 initValue, uint32 fenceFlags) override;
    
        RHIVertexBufferRef RHICreateVertexBuffer(uint32 sizeInBytes, uint32 strideInBytes, EBufferCreateFlags usage, void *resourceData = nullptr) override;
        
        RHIIndexBufferRef RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, EBufferCreateFlags usage, void *resourceData = nullptr) override;
    
        RHIStructuredBufferRef RHICreateStructuredBuffer(uint32 size, EBufferCreateFlags usage, void *resourceData = nullptr) override;
    
        RHIConstantBufferRef RHICreateConstantBuffer(uint32 size, EBufferCreateFlags usage, void *resourceData = nullptr) override;
    
        RHITextureRef RHICreateTexture(const RHIResourceDescriptor& desc, ETextureCreateFlags usage, void *resourceData = nullptr) override;

        bool RHIUpdateSharedMemoryResource(RHIResource* resource, void* resourceData, uint32 size, uint8 subresourceId) override;

        void RHIReleaseResource() override;

        /// dx12 only
        
        ComPtr<ID3D12Device> GetDevice() const { return Device; }
        ID3D12CommandQueue* RHIGetD3DCommandQueue() const { return CommandQueue.Get(); }

        void AddReleaseObject(ComPtr<ID3D12Object> object);

        void RHISignalFence(uint32 frameIndex) override;
        void RHIWaitForFrame(uint32 frameIndex) override;

        // Online descriptor manager for global heap (GPU-visible, transient)
        class D3D12OnlineDescriptorManager* GetOnlineDescriptorManager() const { return OnlineDescriptorManager; }

        // Offline descriptor managers (CPU-side, persistent)
        class D3D12OfflineDescriptorManager* GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

        // Fence utilities
        uint64 GetCurrentFenceValue() const { return CurrentFenceValue; }
        bool IsFenceComplete(uint64 fenceValue) const;

    private:
        void InitializeOnlineDescriptorManager();
        void InitializeOfflineDescriptorManagers();

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

        //
        TArray<ComPtr<ID3D12Object>> GReleaseQueue[MAX_FRAME_LAG] {};
    };
}
