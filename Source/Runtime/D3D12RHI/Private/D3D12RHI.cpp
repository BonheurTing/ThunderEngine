#pragma optimize("", off)
#include "D3D12RHI.h"
#include "D3D12Resource.h"
#include "UniformBuffer.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12CommandContext.h"
#include "D3D12RHIPrivate.h"
#include "D3D12Util.h"
#include "D3D12PipelineState.h"
#include "RHIHelper.h"
#include "Assertion.h"
#include <dxgi1_4.h>

#include "D3D12RHIModule.h"
#include "PlatformProcess.h"
#define INITGUID
#include "D3D12Allocator.h"
#include "d3dx12.h"
#include "RHICommand.h"
#include "Concurrent/TaskScheduler.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
    D3D12DynamicRHI::D3D12DynamicRHI()
    {
        FenceEvent = nullptr;
        CurrentFenceValue = 0;
        for (unsigned long long& FenceValue : ExpectedFenceValues)
        {
            FenceValue = 0;
        }
    }

    D3D12DynamicRHI::~D3D12DynamicRHI()
    {
        for (int i = 0; i < 4; ++i)
        {
            if (OfflineDescriptorManagers[i] != nullptr)
            {
                TMemory::Destroy(OfflineDescriptorManagers[i]);
                OfflineDescriptorManagers[i] = nullptr;
            }
        }

        if (OnlineDescriptorManager != nullptr)
        {
            TMemory::Destroy(OnlineDescriptorManager);
            OnlineDescriptorManager = nullptr;
        }

        if (TransientUploadAllocator)
        {
            delete TransientUploadAllocator;
            TransientUploadAllocator = nullptr;
        }

        if (UploadHeapAllocator)
        {
            delete UploadHeapAllocator;
            UploadHeapAllocator = nullptr;
        }

        if (FenceEvent != nullptr)
        {
            FPlatformProcess::ReturnSyncEventToPool(FenceEvent);
        }
    }

    RHIDeviceRef D3D12DynamicRHI::RHICreateDevice()
    {
        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
    
        /*IDXGIAdapter* warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
    
        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(TempDevice)
            ));*/
    
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);
    
        if(hardwareAdapter == nullptr)
        {
            LOG("Fail to get valid adapter");
            return nullptr;
        }
    
        //ID3D12Device* Device;
        HRESULT hr = D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&Device)
            );
        
        // Initialize offline descriptor managers (CPU-side, for creating descriptors)
        InitializeOfflineDescriptorManagers();

        // Initialize online descriptor manager (GPU-visible, for runtime binding)
        InitializeOnlineDescriptorManager();

        // Initialize null descriptors for binding empty slots
        InitializeNullDescriptors();

        UploadHeapAllocator = new D3D12PersistentUploadHeapAllocator(Device.Get());
        TransientUploadAllocator = new D3D12TransientUploadHeapAllocator(Device.Get());

        TD3D12RHIModule::GetModule()->InitD3D12Context(Device.Get());

        // Create fence for CPU-GPU synchronization
        HRESULT fenceHr = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
        TAssertf(SUCCEEDED(fenceHr), "Failed to create fence");

        // Create event for fence waiting
        FenceEvent = FPlatformProcess::GetSyncEventFromPool(false);
        TAssertf(FenceEvent != nullptr, "Failed to create fence event");

        if(SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12Device>(Device.Get());
        }
        else
        {
            LOG("Fail to create device");
            return nullptr;
        }
    }

    void D3D12DynamicRHI::RHICreateCommandQueue()
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        HRESULT hr = Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue));
        if (FAILED(hr))
        {
            TAssertf(false, "Fail to create command queue");
        }
    }

    RHICommandContextRef D3D12DynamicRHI::RHICreateCommandContext()
    {
        ComPtr<ID3D12CommandAllocator> CommandAllocator[MAX_FRAME_LAG];
        ComPtr<ID3D12GraphicsCommandList> CommandList;

        HRESULT hr;
        for (auto& i : CommandAllocator)
        {
            hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&i));
            if (FAILED(hr))
            {
                TAssertf(false, "Fail to create command allocator");
            }
        }
        hr = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator[0].Get(), nullptr, IID_PPV_ARGS(&CommandList));
        if (FAILED(hr))
        {
            TAssertf(false, "Fail to create command list");
        }

        return MakeRefCount<D3D12CommandContext>(CommandAllocator, CommandList.Get());
    }

    TRHIGraphicsPipelineState* D3D12DynamicRHI::RHICreateGraphicsPipelineState(TGraphicsPipelineStateDescriptor& initializer)
    {
        TD3D12PipelineStateCache* psoCache = TD3D12RHIModule::GetModule()->GetPipelineStateTable();
        return psoCache->FindOrCreateGraphicsPipelineState(initializer);
    }

    void D3D12DynamicRHI::RHICreateComputePipelineState()
    {
        
    }

    static inline uint32 Align256(uint32 size)
    {
        return (size + 255u) & ~255u;
    }

    void D3D12DynamicRHI::RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize)
    {
        TAssert(Device != nullptr);

        // Allocate from offline descriptor manager (CPU-side, persistent)
        auto* offlineManager = GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12OfflineDescriptor cbvDescriptor = offlineManager->AllocateHeapSlot();

        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = inst->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = Align256(bufferSize);
        Device->CreateConstantBufferView(&cbvDesc, cbvDescriptor.Handle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            D3D12RHIConstantBufferView* view = new D3D12RHIConstantBufferView(cbvDescriptor);
            resource.SetCBV(view);
        }
        else
        {
            TAssertf(false, "Fail to create constant buffer view");
        }
    }

    void D3D12DynamicRHI::RHICreateShaderResourceView(RHIResource& resource, const RHIViewDescriptor& desc)
    {
        TAssert(Device != nullptr);

        // Allocate from offline descriptor manager (CPU-side, persistent)
        auto* offlineManager = GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12OfflineDescriptor srvDescriptor = offlineManager->AllocateHeapSlot();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        // Depth-stencil textures are created with a typeless format; SRV must use the readable depth format.
        DXGI_FORMAT srvFormat = ConvertRHIFormatToD3DFormat(desc.Format);
        if (resource.GetResourceDescriptor()->Flags.NeedDSV)
        {
            srvFormat = GetDepthSRVFormat(ConvertRHIFormatToD3DFormat(resource.GetResourceDescriptor()->Format));
        }
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = ConvertToD3D12SRVDimension(desc.Type);
        const auto resourceDesc = resource.GetResourceDescriptor();
        switch (resourceDesc->Type)
        {
        case ERHIResourceType::Buffer:
            srvDesc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
            break;
        case ERHIResourceType::Texture1D:
            srvDesc.Texture1D.MipLevels = resourceDesc->MipLevels;
            break;
        case ERHIResourceType::Texture2D:
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = resourceDesc->MipLevels;
            break;
        case ERHIResourceType::Texture2DArray:
            //todo: detail desc
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = resourceDesc->MipLevels;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Texture3D:
            srvDesc.Texture3D.MostDetailedMip = 0;
            srvDesc.Texture3D.MipLevels = resourceDesc->MipLevels;
            break;
        case ERHIResourceType::Unknown: break;
        }

        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        Device->CreateShaderResourceView(inst, &srvDesc, srvDescriptor.Handle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            // Store the CPU handle ptr (will be used for copying to online heap during draw)
            D3D12RHIShaderResourceView* view = new D3D12RHIShaderResourceView(desc, srvDescriptor);
            resource.SetSRV(view);
        }
        else
        {
            TAssertf(false, "Fail to create shader resource view");
        }
    }

    void D3D12DynamicRHI::RHICreateUnorderedAccessView(RHIResource& resource, const RHIViewDescriptor& desc)
    {
        TAssert(Device != nullptr);

        // Allocate from offline descriptor manager (CPU-side, persistent)
        auto* offlineManager = GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12OfflineDescriptor uavDescriptor = offlineManager->AllocateHeapSlot();

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        const auto resourceDesc = resource.GetResourceDescriptor();
        switch (resourceDesc->Type)
        {
        case ERHIResourceType::Buffer:
            uavDesc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
            break;
        case ERHIResourceType::Texture1D:
            uavDesc.Texture1D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2D:
            uavDesc.Texture2D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2DArray:
            uavDesc.Texture2DArray.MipSlice = 0;
            uavDesc.Texture2DArray.FirstArraySlice = 0;
            uavDesc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
            uavDesc.Texture2DArray.PlaneSlice = 0;
            break;
        case ERHIResourceType::Texture3D:
            uavDesc.Texture3D.MipSlice = 0;
            uavDesc.Texture3D.FirstWSlice = 0;
            uavDesc.Texture3D.WSize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Unknown: break;
        }

        uavDesc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
        uavDesc.ViewDimension = ConvertToD3D12UAVDimension(desc.Type);
        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        Device->CreateUnorderedAccessView(inst, nullptr, &uavDesc, uavDescriptor.Handle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            D3D12RHIUnorderedAccessView* view = new D3D12RHIUnorderedAccessView(desc, uavDescriptor);
            resource.SetUAV(view);
        }
        else
        {
            TAssertf(false, "Fail to create unordered access view");
        }
    }

    void D3D12DynamicRHI::RHICreateRenderTargetView(RHITexture& resource, const RHIViewDescriptor& desc)
    {
        TAssert(Device != nullptr);

        // Allocate from offline descriptor manager (CPU-side, persistent)
        auto* offlineManager = GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12OfflineDescriptor rtvDescriptor = offlineManager->AllocateHeapSlot();

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
        rtvDesc.ViewDimension = ConvertToD3D12RTVDimension(desc.Type);
        const auto resourceDesc = resource.GetResourceDescriptor();
        switch (resourceDesc->Type)
        {
        case ERHIResourceType::Buffer:
            rtvDesc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
            break;
        case ERHIResourceType::Texture1D:
            rtvDesc.Texture1D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2D:
            rtvDesc.Texture2D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2DArray:
            rtvDesc.Texture2DArray.MipSlice = 0;
            rtvDesc.Texture2DArray.FirstArraySlice = 0;
            rtvDesc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Texture3D:
            rtvDesc.Texture3D.MipSlice = 0;
            rtvDesc.Texture3D.FirstWSlice = 0;
            rtvDesc.Texture3D.WSize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Unknown: break;
        }

        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        Device->CreateRenderTargetView(inst, &rtvDesc, rtvDescriptor.Handle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            D3D12RHIRenderTargetView* view = new D3D12RHIRenderTargetView(desc, rtvDescriptor);
            resource.SetRTV(view);
        }
        else
        {
            TAssertf(false, "Fail to create render target view");
        }
    }

    void D3D12DynamicRHI::RHICreateDepthStencilView(RHITexture& resource, const RHIViewDescriptor& desc)
    {
        TAssert(Device != nullptr);

        // Allocate from offline descriptor manager (CPU-side, persistent)
        auto* offlineManager = GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        D3D12OfflineDescriptor dsvDescriptor = offlineManager->AllocateHeapSlot();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
        dsvDesc.ViewDimension = ConvertToD3D12DSVDimension(desc.Type);
        const auto resourceDesc = resource.GetResourceDescriptor();
        switch (resourceDesc->Type)
        {
        case ERHIResourceType::Buffer:
            break;
        case ERHIResourceType::Texture1D:
            dsvDesc.Texture1D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2D:
            dsvDesc.Texture2D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2DArray:
            dsvDesc.Texture2DArray.MipSlice = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = 0;
            dsvDesc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Texture3D:
        case ERHIResourceType::Unknown: break;
        }

        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        Device->CreateDepthStencilView(inst, &dsvDesc, dsvDescriptor.Handle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            D3D12RHIDepthStencilView* view = new D3D12RHIDepthStencilView(desc, dsvDescriptor);
            resource.SetDSV(view);
        }
        else
        {
            TAssertf(false, "Fail to create depth stencil view");
        }
    }

    RHISamplerRef D3D12DynamicRHI::RHICreateSampler(const RHISamplerDescriptor& desc)
    {
        TAssert(Device != nullptr);

        // Allocate from offline descriptor manager (CPU-side, persistent)
        auto* offlineManager = GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        D3D12OfflineDescriptor samplerDescriptor = offlineManager->AllocateHeapSlot();

        D3D12_SAMPLER_DESC samplerDesc;
        samplerDesc.Filter = static_cast<D3D12_FILTER>(desc.Filter);
        samplerDesc.AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(desc.AddressU);
        samplerDesc.AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(desc.AddressV);
        samplerDesc.AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(desc.AddressW);
        samplerDesc.MipLODBias = desc.MipLODBias;
        samplerDesc.MaxAnisotropy = desc.MaxAnisotropy;
        samplerDesc.ComparisonFunc = static_cast<D3D12_COMPARISON_FUNC>(desc.ComparisonFunc);
        samplerDesc.BorderColor[0] = desc.BorderColor[0];
        samplerDesc.BorderColor[1] = desc.BorderColor[1];
        samplerDesc.BorderColor[2] = desc.BorderColor[2];
        samplerDesc.BorderColor[3] = desc.BorderColor[3];
        samplerDesc.MinLOD = desc.MinLOD;
        samplerDesc.MaxLOD = desc.MaxLOD;

        Device->CreateSampler(&samplerDesc, samplerDescriptor.Handle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHISampler>(desc, samplerDescriptor);
        }
        else
        {
            TAssertf(false, "Fail to create sampler");
            return nullptr;
        }
    }

    RHIFenceRef D3D12DynamicRHI::RHICreateFence(uint64 initValue, uint32 fenceFlags)
    {
        TAssert(Device != nullptr);
        ComPtr<ID3D12Fence> fence;
        const HRESULT hr = Device->CreateFence(initValue, static_cast<D3D12_FENCE_FLAGS>(fenceFlags), IID_PPV_ARGS(&fence));

        if(SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIFence>(initValue, fence);
        }
        else
        {
            TAssertf(false, "Fail to create fence");
            return nullptr;
        }
    }

    RHIVertexBufferRef D3D12DynamicRHI::RHICreateVertexBuffer(uint32 sizeInBytes, uint32 strideInBytes,  EBufferCreateFlags usage, void *resourceData)
    {
        ID3D12Resource* vertexBuffer;
        const bool bDynamic = EnumHasAnyFlags(usage, EBufferCreateFlags::AnyDynamic);
        const D3D12_HEAP_PROPERTIES heapType = {
            bDynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };

        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &desc,
                                                           D3D12_RESOURCE_STATE_COMMON,
                                                           nullptr,
                                                           IID_PPV_ARGS(&vertexBuffer));

        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIVertexBuffer>(sizeInBytes, strideInBytes, RHIResourceDescriptor::Buffer(sizeInBytes), usage, vertexBuffer);
        }
        else
        {
            TAssertf(false, "Fail to create vertex buffer (size: %u, dynamic: %d, HRESULT: 0x%08X).", sizeInBytes, bDynamic, static_cast<uint32>(hr));
            return nullptr;
        }
    }
    
    RHIIndexBufferRef D3D12DynamicRHI::RHICreateIndexBuffer(uint32 width, ERHIIndexBufferType type, EBufferCreateFlags usage, void *resourceData)
    {
        ID3D12Resource* indexBuffer;
        const D3D12_HEAP_PROPERTIES heapType = {
            EnumHasAnyFlags(usage, EBufferCreateFlags::AnyDynamic) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };
        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(width);
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &desc,
                                                           D3D12_RESOURCE_STATE_COMMON,
                                                           nullptr,
                                                           IID_PPV_ARGS(&indexBuffer));

        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIIndexBuffer>(type, RHIResourceDescriptor::Buffer(width), usage, indexBuffer);
        }
        else
        {
            TAssertf(false, "Fail to create index buffer");
            return nullptr;
        }
    }
    
    RHIStructuredBufferRef D3D12DynamicRHI::RHICreateStructuredBuffer(uint32 size,  EBufferCreateFlags usage, void *resourceData)
    {
        ID3D12Resource* structuredBuffer;
        const D3D12_HEAP_PROPERTIES heapType = {
            EnumHasAnyFlags(usage, EBufferCreateFlags::AnyDynamic) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };

        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &desc,
                                                            D3D12_RESOURCE_STATE_COMMON,
                                                            nullptr,
                                                            IID_PPV_ARGS(&structuredBuffer));

        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIStructuredBuffer>(RHIResourceDescriptor::Buffer(size), structuredBuffer);
        }
        else
        {
            TAssertf(false, "Fail to create structured buffer");
            return nullptr;
        }
    }
    
    RHIConstantBufferRef D3D12DynamicRHI::RHICreateConstantBuffer(uint32 size, EBufferCreateFlags usage, void *resourceData)
    {
        ID3D12Resource* constantBuffer;
        constexpr D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};

        const uint32 alignedSize = Align256(size);
        
        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &desc, //todo: D3D12_HEAP_FLAG_NONE
                                                            D3D12_RESOURCE_STATE_COMMON,
                                                            nullptr,
                                                            IID_PPV_ARGS(&constantBuffer));
    
        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIConstantBuffer>(RHIResourceDescriptor::Buffer(alignedSize), constantBuffer);
        }
        else
        {
            TAssertf(false, "Fail to create constant buffer");
            return nullptr;
        }
    }

    RHIUniformBufferRef D3D12DynamicRHI::RHICreateUniformBuffer(uint32 size, EUniformBufferFlags usage, const void* Contents)
    {
        const uint32 alignedSize = Align256(size);

        D3D12UniformBuffer* newUniformBuffer = new D3D12UniformBuffer(usage);

        if (Contents)
        {
            void* MappedData = nullptr;
            if (usage == EUniformBufferFlags::UniformBuffer_MultiFrame)
            {
                // Uniform buffers that live for multiple frames must use the more expensive and persistent allocation path
                MappedData = UploadHeapAllocator->Allocate(alignedSize, newUniformBuffer->ResourceLocation);
            }
            else
            {
                // SingleFrame / SingleDraw: use transient linear allocator, reset each frame
                MappedData = TransientUploadAllocator->Allocate(alignedSize, newUniformBuffer->ResourceLocation);
            }

            memcpy(MappedData, Contents, alignedSize);
        }
        else
        {
            TAssertf(false, "Fail to create uniform buffer.");
        }

        return newUniformBuffer;
    }

    struct RHICommandD3D12UpdateUniformBuffer : public IRHICommand
    {
        TRefCountPtr<D3D12UniformBuffer> UniformBuffer;
        D3D12ResourceLocation UpdatedLocation;

        RHICommandD3D12UpdateUniformBuffer(D3D12UniformBuffer* InUniformBuffer, D3D12ResourceLocation& InUpdatedLocation)
            : UniformBuffer(InUniformBuffer)
        {
            D3D12ResourceLocation::TransferOwnership(UpdatedLocation, InUpdatedLocation);
        }

        ~RHICommandD3D12UpdateUniformBuffer() override {}

        D3D12RHI_API void Execute(RHICommandContext* cmdList) override
        {
            D3D12ResourceLocation::TransferOwnership(UniformBuffer->ResourceLocation, UpdatedLocation);
        }
    };

    void D3D12DynamicRHI::RHIUpdateUniformBuffer(IRHICommandRecorder* recorder, RHIUniformBuffer* uniformBuffer, const void* Contents)
    {
        // in render thread
        D3D12UniformBuffer* dx12UB = static_cast<D3D12UniformBuffer*>(uniformBuffer);
        const uint32 alignedSize = dx12UB->ResourceLocation.AllocatedSize;

        D3D12ResourceLocation UpdatedResourceLocation;
        if (alignedSize > 0)
        {
            void* mappedData = nullptr;

            if (uniformBuffer->UniformBufferUsage == EUniformBufferFlags::UniformBuffer_MultiFrame)
            {
                mappedData = UploadHeapAllocator->Allocate(alignedSize, UpdatedResourceLocation);

                uint32 currentFrame = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
                if (dx12UB->ResourceLocation.IsValid())
                {
                    UploadHeapAllocator->DeferredFree(dx12UB->ResourceLocation, currentFrame);
                }
            }
            else
            {
                // SingleFrame / SingleDraw: allocate from transient allocator.
                // No need to deferred-free the old location; it will be reclaimed on frame flip.
                mappedData = TransientUploadAllocator->Allocate(alignedSize, UpdatedResourceLocation);
            }

            memcpy(mappedData, Contents, alignedSize);
        }

        RHICommandD3D12UpdateUniformBuffer* newCommand = new (recorder->Allocate<RHICommandD3D12UpdateUniformBuffer>()) RHICommandD3D12UpdateUniformBuffer(dx12UB, UpdatedResourceLocation);
        recorder->AddCommand(newCommand);
    }

    RHITextureRef D3D12DynamicRHI::RHICreateTexture(const RHIResourceDescriptor& desc, ETextureCreateFlags usage, void *resourceData)
    {
        ID3D12Resource* texture = nullptr;
        
        // Configure common heap properties
        constexpr D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};
        
        // Configure common resource desc
        D3D12_RESOURCE_DESC d3d12Desc = {};
        d3d12Desc.Alignment = desc.Alignment;
        d3d12Desc.Width = desc.Width;
        d3d12Desc.Height = desc.Height;
        d3d12Desc.MipLevels = desc.MipLevels;
        // Depth-stencil textures that also need an SRV must be created with a typeless format.
        DXGI_FORMAT resourceFormat = ConvertRHIFormatToD3DFormat(desc.Format);
        if (desc.Flags.NeedDSV)
        {
            resourceFormat = GetDepthTypelessFormat(resourceFormat);
        }
        d3d12Desc.Format = resourceFormat;
        d3d12Desc.SampleDesc = {1, 0};
        d3d12Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        d3d12Desc.Flags = GetRHIResourceFlags(desc.Flags);

        switch (desc.Type)
        {
        case ERHIResourceType::Texture1D:
        {
            TAssertf(desc.Height == 1, "Height should be 1 for Texture1D.");
            TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Texture1D.");
            
            d3d12Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            d3d12Desc.Height = 1;
            d3d12Desc.DepthOrArraySize = 1;
            break;
        }
        case ERHIResourceType::Texture2D:
        {
            TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Texture2D.");
            
            d3d12Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            d3d12Desc.DepthOrArraySize = 1;
            break;
        }
        case ERHIResourceType::Texture2DArray:
        {
            d3d12Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            d3d12Desc.DepthOrArraySize = desc.DepthOrArraySize;
            break;
        }
        case ERHIResourceType::Texture3D:
        {
            // Note: D3D12 uses TEXTURE3D dimension for 3D textures, not TEXTURE2D
            d3d12Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            d3d12Desc.DepthOrArraySize = desc.DepthOrArraySize;
            break;
        }
        default:
            TAssertf(false, "Unsupported texture type");
            return nullptr;
        }

        D3D12_CLEAR_VALUE  optimizedClearValue{};
        D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr;
        if (desc.bHasOptimizedClearValue && (desc.Flags.NeedRTV || desc.Flags.NeedDSV))
        {
            if (desc.Flags.NeedDSV)
            {
                optimizedClearValue.Format               = ConvertRHIFormatToD3DFormat(desc.Format);
                optimizedClearValue.DepthStencil.Depth   = desc.OptimizedClearDepth;
                optimizedClearValue.DepthStencil.Stencil = desc.OptimizedClearStencil;
            }
            else
            {
                optimizedClearValue.Format   = ConvertRHIFormatToD3DFormat(desc.Format);
                optimizedClearValue.Color[0] = desc.OptimizedClearColor[0];
                optimizedClearValue.Color[1] = desc.OptimizedClearColor[1];
                optimizedClearValue.Color[2] = desc.OptimizedClearColor[2];
                optimizedClearValue.Color[3] = desc.OptimizedClearColor[3];
            }
            pOptimizedClearValue = &optimizedClearValue;
        }

        HRESULT hr = Device->CreateCommittedResource(&heapType,
                                               D3D12_HEAP_FLAG_NONE,
                                               &d3d12Desc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                               pOptimizedClearValue,
                                               IID_PPV_ARGS(&texture));
        
        if(SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHITexture>(desc, usage, texture);
        }
        else
        {
            if (hr == DXGI_ERROR_DEVICE_REMOVED)
            {
                HRESULT removeReason = Device->GetDeviceRemovedReason();
                if (removeReason == DXGI_ERROR_DEVICE_REMOVED)
                {
                    TAssertf(false, "Fail to create texture");
                }
            }
            TAssertf(false, "Fail to create texture");
            return nullptr;
        }
    }

    bool D3D12DynamicRHI::RHIUpdateSharedMemoryResource(RHIResource* resource, const void* resourceData, uint32 size, uint8 subresourceId)
    {
        if (auto const d3d12Res = static_cast<ID3D12Resource*>(resource->GetResource()))
        {
            UINT8* pVertexDataBegin;
            const auto hr = d3d12Res->Map(subresourceId, nullptr, reinterpret_cast<void**>(&pVertexDataBegin));
            if (FAILED(hr))
            {
                TAssertf(false, "Fail to map resource");
                return false;
            }
            memcpy(pVertexDataBegin, resourceData, size);
            d3d12Res->Unmap(subresourceId, nullptr);
            return true;
        }
        return false;
    }

    void D3D12DynamicRHI::RHIReleaseResource_RenderThread()
    {
        uint32 index = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
        UploadHeapAllocator->GarbageCollect(index);
        TransientUploadAllocator->FrameFlip(index);
    }

    void D3D12DynamicRHI::RHIReleaseResource_RHIThread()
    {
        uint32 index = (GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire) + 1) % ReleaseQueueLength;
        if (!ReleaseQueue[index].empty())
        {
            TArray<ComPtr<ID3D12Object>> releaseQueue;
            releaseQueue.swap(ReleaseQueue[index]);
            GAsyncWorkers->PushTask([releaseQueue = std::move(releaseQueue)]() mutable
            {
            });
        }
    }

    void D3D12DynamicRHI::AddReleaseObject(ComPtr<ID3D12Object> object)
    {
        GRHIScheduler->PushTask([this, object]()
        {
            uint32 const rhiFrameCount = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire);
            uint32 index = rhiFrameCount % ReleaseQueueLength;
            ReleaseQueue[index].push_back(object);
        });
    }

    void D3D12DynamicRHI::RHISignalFence(uint32 frameIndex)
    {
        ++CurrentFenceValue;
        ExpectedFenceValues[frameIndex] = CurrentFenceValue;

        HRESULT hr = CommandQueue->Signal(Fence.Get(), CurrentFenceValue);
        TAssertf(SUCCEEDED(hr), "Failed to signal fence");
    }

    void D3D12DynamicRHI::RHIBeginFrame(uint32 frameIndex)
    {
        if (OnlineDescriptorManager != nullptr)
        {
            OnlineDescriptorManager->ProcessDeferredDeletions();
        }

        // Check for pending resize request and execute it on RHI thread
        // ResizeBackBuffer_RHIThread();
    }

    void D3D12DynamicRHI::RHIWaitForFrame(uint32 frameIndex)
    {
        uint64 fenceValue = ExpectedFenceValues[frameIndex];

        if (fenceValue == 0)
        {
            return; // No fence value recorded yet
        }

        uint64 completedValue = Fence->GetCompletedValue();
        if (completedValue < fenceValue)
        {
            HRESULT hr = Fence->SetEventOnCompletion(fenceValue, static_cast<HANDLE>(FenceEvent->GetNativeHandle()));
            TAssertf(SUCCEEDED(hr), "Failed to set fence event");
            FenceEvent->Wait();
        }
    }

    void D3D12DynamicRHI::InitializeOnlineDescriptorManager()
    {
        // Configuration: 512K total, 2K block
        constexpr uint32 kGlobalHeapSize = 524288; // 512K descriptors
        constexpr uint32 kBlockSize = 2048;        // 2K descriptors per block

        OnlineDescriptorManager = new (TMemory::Malloc<D3D12OnlineDescriptorManager>())
            D3D12OnlineDescriptorManager(Device.Get());

        OnlineDescriptorManager->Init(kGlobalHeapSize, kBlockSize);
    }

    void D3D12DynamicRHI::InitializeOfflineDescriptorManagers()
    {
        // Heap sizes.
        constexpr uint32 kStandardHeapSize = 4096;  // CBV_SRV_UAV
        constexpr uint32 kRTVHeapSize = 256;
        constexpr uint32 kDSVHeapSize = 256;
        constexpr uint32 kSamplerHeapSize = 128;

        // Create offline descriptor managers for each type
        OfflineDescriptorManagers[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] =
            new (TMemory::Malloc<D3D12OfflineDescriptorManager>())
                D3D12OfflineDescriptorManager(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kStandardHeapSize);

        OfflineDescriptorManagers[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] =
            new (TMemory::Malloc<D3D12OfflineDescriptorManager>())
                D3D12OfflineDescriptorManager(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kRTVHeapSize);

        OfflineDescriptorManagers[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] =
            new (TMemory::Malloc<D3D12OfflineDescriptorManager>())
                D3D12OfflineDescriptorManager(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kDSVHeapSize);

        OfflineDescriptorManagers[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] =
            new (TMemory::Malloc<D3D12OfflineDescriptorManager>())
                D3D12OfflineDescriptorManager(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, kSamplerHeapSize);
    }

    D3D12OfflineDescriptorManager* D3D12DynamicRHI::GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE type) const
    {
        TAssert(type >= 0 && type < 4);
        return OfflineDescriptorManagers[type];
    }

    void D3D12DynamicRHI::InitializeNullDescriptors()
    {
        // Create null descriptors for SRV, UAV, and CBV
        // These are used when binding empty slots in descriptor tables

        D3D12OfflineDescriptorManager* manager = GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Allocate slots from offline heap
        D3D12OfflineDescriptor nullSrvDesc = manager->AllocateHeapSlot();
        D3D12OfflineDescriptor nullUavDesc = manager->AllocateHeapSlot();
        D3D12OfflineDescriptor nullCbvDesc = manager->AllocateHeapSlot();

        // Create null SRV (Texture2D)
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        Device->CreateShaderResourceView(nullptr, &srvDesc, nullSrvDesc.Handle);

        // Create null UAV (Texture2D)
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        Device->CreateUnorderedAccessView(nullptr, nullptr, &uavDesc, nullUavDesc.Handle);

        // Create null CBV
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = 0;
        cbvDesc.SizeInBytes = 256; // Minimum size
        Device->CreateConstantBufferView(&cbvDesc, nullCbvDesc.Handle);

        // Store the handles
        NullSRV = nullSrvDesc.Handle;
        NullUAV = nullUavDesc.Handle;
        NullCBV = nullCbvDesc.Handle;
    }

    bool D3D12DynamicRHI::IsFenceComplete(uint64 fenceValue) const
    {
        return Fence->GetCompletedValue() >= fenceValue;
    }

    void D3D12DynamicRHI::RHICreateSwapChain(void* hwnd, uint32 width, uint32 height)
    {
        TAssertf(CommandQueue != nullptr, "CommandQueue must be created before SwapChain.");

        NativeHwnd = static_cast<HWND>(hwnd);

        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width       = width;
        desc.Height      = height;
        desc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferCount = 2;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.SampleDesc  = { 1, 0 };

        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            CommandQueue.Get(), NativeHwnd, &desc, nullptr, nullptr, &swapChain1));

        // Disable Alt+Enter fullscreen toggle
        factory->MakeWindowAssociation(NativeHwnd, DXGI_MWA_NO_ALT_ENTER);

        ThrowIfFailed(swapChain1.As(&SwapChain));
        CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

        // Create RTVs for both back buffers
        auto* rtvManager = GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (uint32 i = 0; i < 2; ++i)
        {
            ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffers[i])));
            BackBufferRTVs[i] = rtvManager->AllocateHeapSlot();
            Device->CreateRenderTargetView(SwapChainBuffers[i].Get(), nullptr, BackBufferRTVs[i].Handle);
        }
    }

    void D3D12DynamicRHI::ResizeBackBuffer_RHIThread()
    {
        TVector2u desiredResolution = GetMainViewportResolution_RHIThread();
        if (desiredResolution.X == 0 || desiredResolution.Y == 0)
        {
            return;
        }

        if (CachedSwapChainResolution.X == desiredResolution.X
            && CachedSwapChainResolution.Y == desiredResolution.Y)
        {
            return;
        }

        // Wait for GPU to finish ALL work before resizing
        // This ensures no commands are using the backbuffers
        ++CurrentFenceValue;
        CommandQueue->Signal(Fence.Get(), CurrentFenceValue);
        if (Fence->GetCompletedValue() < CurrentFenceValue)
        {
            Fence->SetEventOnCompletion(CurrentFenceValue, static_cast<HANDLE>(FenceEvent->GetNativeHandle()));
            FenceEvent->Wait();
        }

        // Release back buffer resources and RTVs
        auto* rtvManager = GetOfflineDescriptorManager(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (uint32 i = 0; i < 2; ++i)
        {
            SwapChainBuffers[i].Reset();
            if (BackBufferRTVs[i].Handle.ptr != 0)
            {
                rtvManager->FreeHeapSlot(BackBufferRTVs[i]);
                BackBufferRTVs[i] = {};
            }
        }

        // Resize swap chain buffers
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        SwapChain->GetDesc1(&desc);
        ThrowIfFailed(SwapChain->ResizeBuffers(
            2,
            desiredResolution.X,
            desiredResolution.Y,
            desc.Format,
            desc.Flags));

        // Recreate back buffer RTVs
        CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
        for (uint32 i = 0; i < 2; ++i)
        {
            ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffers[i])));
            BackBufferRTVs[i] = rtvManager->AllocateHeapSlot();
            Device->CreateRenderTargetView(SwapChainBuffers[i].Get(), nullptr, BackBufferRTVs[i].Handle);
        }

        CachedSwapChainResolution = desiredResolution;
    }

    void D3D12DynamicRHI::RHIPresent()
    {
        if (!SwapChain)
        {
            return;
        }
        ThrowIfFailed(SwapChain->Present(1, 0));  // vsync=1
        CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

        // Check for pending resize request and execute it on RHI thread
        ResizeBackBuffer_RHIThread();
    }
}
