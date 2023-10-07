#include "D3D12RHI.h"
#include "D3D12Resource.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12CommandContext.h"
#include "RHIHelper.h"
#include "Assertion.h"
#include <dxgi1_4.h>
#include "d3dx12.h"

namespace Thunder
{
    D3D12DynamicRHI::D3D12DynamicRHI()
    {
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
        
        CommonDescriptorHeap = MakeRefCount<TD3D12DescriptorHeap>(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, CommonDescriptorHeapSize);
        RTVDescriptorHeap = MakeRefCount<TD3D12DescriptorHeap>(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RTVDescriptorHeapSize);
        DSVDescriptorHeap = MakeRefCount<TD3D12DescriptorHeap>(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DSVDescriptorHeapSize);
        SamplerDescriptorHeap = MakeRefCount<TD3D12DescriptorHeap>(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SamplerDescriptorHeapSize);
        
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

    RHICommandContextRef D3D12DynamicRHI::RHICreateCommandContext()
    {
        ComPtr<ID3D12CommandQueue> CommandQueue;
        ComPtr<ID3D12CommandAllocator> CommandAllocator;
        ComPtr<ID3D12GraphicsCommandList> CommandList;
        
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        HRESULT hr = Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue));
        hr = hr && Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator));
        hr = hr && Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&CommandList));

        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12CommandContext>(CommandQueue.Get(), CommandAllocator.Get(), CommandList.Get());
        }
        else
        {
            TAssertf(false, "Fail to Create Command Context");
            return nullptr;
        }
    }

    RHIRasterizerStateRef D3D12DynamicRHI::RHICreateRasterizerState(const RasterizerStateInitializerRHI& Initializer)
    {
        return nullptr;
    }
    
    RHIDepthStencilStateRef D3D12DynamicRHI::RHICreateDepthStencilState(const DepthStencilStateInitializerRHI& Initializer)
    {
        return nullptr;
    }
    
    RHIBlendStateRef D3D12DynamicRHI::RHICreateBlendState(const BlendStateInitializerRHI& Initializer)
    {
        return nullptr;
    }
    
    RHIInputLayoutRef D3D12DynamicRHI::RHICreateInputLayout(const RHIInputLayoutDescriptor& initializer)
    {
        return nullptr;
    }
    
    RHIVertexDeclarationRef D3D12DynamicRHI::RHICreateVertexDeclaration(const VertexDeclarationInitializerRHI& Elements)
    {
        return nullptr;
    }

    void D3D12DynamicRHI::RHICreateConstantBufferView(RHIBuffer& resource, uint32 bufferSize)
    {
        TAssert(Device != nullptr);
        uint32 cbvOffset = 0;
        const D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle{ CommonDescriptorHeap->AllocateDescriptorHeap(cbvOffset) };

        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        D3D12_CONSTANT_BUFFER_VIEW_DESC dx12Desc;
        dx12Desc.BufferLocation = inst->GetGPUVirtualAddress();
        dx12Desc.SizeInBytes = 256;//bufferSize + 256 - bufferSize % 256;
        Device->CreateConstantBufferView(&dx12Desc, cbvHandle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            const D3D12RHIConstantBufferView view{CommonDescriptorHeap.get(), cbvOffset};
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
        uint32 srvOffset;
        const D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{ CommonDescriptorHeap->AllocateDescriptorHeap(srvOffset) };

        D3D12_SHADER_RESOURCE_VIEW_DESC dx12Desc{};
        dx12Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        dx12Desc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
        dx12Desc.ViewDimension = static_cast<D3D12_SRV_DIMENSION>(desc.Type);
        const auto resourceDesc = resource.GetResourceDescriptor();
        switch (resourceDesc->Type)
        {
        case ERHIResourceType::Buffer:
            dx12Desc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
            break;
        case ERHIResourceType::Texture1D:
            dx12Desc.Texture1D.MipLevels = resourceDesc->MipLevels;
            break;
        case ERHIResourceType::Texture2D:
            dx12Desc.Texture2D.MostDetailedMip = 0;
            dx12Desc.Texture2D.MipLevels = resourceDesc->MipLevels;
            break;
        case ERHIResourceType::Texture2DArray:
            //todo: detail desc
            dx12Desc.Texture2DArray.MostDetailedMip = 0;
            dx12Desc.Texture2DArray.MipLevels = resourceDesc->MipLevels;
            dx12Desc.Texture2DArray.FirstArraySlice = 0;
            dx12Desc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Texture3D:
            dx12Desc.Texture3D.MostDetailedMip = 0;
            dx12Desc.Texture3D.MipLevels = resourceDesc->MipLevels;
            break;
        case ERHIResourceType::Unknown: break;
        }
        
        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        Device->CreateShaderResourceView(inst, &dx12Desc, srvHandle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            const D3D12RHIShaderResourceView view{desc, CommonDescriptorHeap.get(), srvOffset};
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
        uint32 uavOffset;
        const D3D12_CPU_DESCRIPTOR_HANDLE uavHandle{ CommonDescriptorHeap->AllocateDescriptorHeap(uavOffset) };

        D3D12_UNORDERED_ACCESS_VIEW_DESC dx12Desc{};
        const auto resourceDesc = resource.GetResourceDescriptor();
        switch (resourceDesc->Type)
        {
        case ERHIResourceType::Buffer:
            dx12Desc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
            break;
        case ERHIResourceType::Texture1D:
            dx12Desc.Texture1D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2D:
            dx12Desc.Texture2D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2DArray:
            dx12Desc.Texture2DArray.MipSlice = 0;
            dx12Desc.Texture2DArray.FirstArraySlice = 0;
            dx12Desc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
            dx12Desc.Texture2DArray.PlaneSlice = 0;
            break;
        case ERHIResourceType::Texture3D:
            dx12Desc.Texture3D.MipSlice = 0;
            dx12Desc.Texture3D.FirstWSlice = 0;
            dx12Desc.Texture3D.WSize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Unknown: break;
        }
        
        dx12Desc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
        dx12Desc.ViewDimension = static_cast<D3D12_UAV_DIMENSION>(desc.Type);
        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        Device->CreateUnorderedAccessView(inst, nullptr, &dx12Desc, uavHandle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            const D3D12RHIUnorderedAccessView view{desc, CommonDescriptorHeap.get(), uavOffset};
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
        uint32 rtvOffset;
        const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{ RTVDescriptorHeap->AllocateDescriptorHeap(rtvOffset) };

        D3D12_RENDER_TARGET_VIEW_DESC dx12Desc{};
        dx12Desc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
        dx12Desc.ViewDimension = static_cast<D3D12_RTV_DIMENSION>(desc.Type);
        const auto resourceDesc = resource.GetResourceDescriptor();
        switch (resourceDesc->Type)
        {
        case ERHIResourceType::Buffer:
            dx12Desc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
            break;
        case ERHIResourceType::Texture1D:
            dx12Desc.Texture1D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2D:
            dx12Desc.Texture2D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2DArray:
            dx12Desc.Texture2DArray.MipSlice = 0;
            dx12Desc.Texture2DArray.FirstArraySlice = 0;
            dx12Desc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Texture3D:
            dx12Desc.Texture3D.MipSlice = 0;
            dx12Desc.Texture3D.FirstWSlice = 0;
            dx12Desc.Texture3D.WSize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Unknown: break;
        }
        
        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        Device->CreateRenderTargetView(inst, &dx12Desc, rtvHandle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            const D3D12RHIRenderTargetView view{desc, RTVDescriptorHeap.get(), rtvOffset};
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
        uint32 dsvOffset;
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{ DSVDescriptorHeap->AllocateDescriptorHeap(dsvOffset) };

        D3D12_DEPTH_STENCIL_VIEW_DESC dx12Desc{};
        dx12Desc.Format = ConvertRHIFormatToD3DFormat(desc.Format);
        dx12Desc.ViewDimension = static_cast<D3D12_DSV_DIMENSION>(desc.Type);
        const auto resourceDesc = resource.GetResourceDescriptor();
        switch (resourceDesc->Type)
        {
        case ERHIResourceType::Buffer:
            break;
        case ERHIResourceType::Texture1D:
            dx12Desc.Texture1D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2D:
            dx12Desc.Texture2D.MipSlice = 0;
            break;
        case ERHIResourceType::Texture2DArray:
            dx12Desc.Texture2DArray.MipSlice = 0;
            dx12Desc.Texture2DArray.FirstArraySlice = 0;
            dx12Desc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Texture3D:
        case ERHIResourceType::Unknown: break;
        }
        
        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        Device->CreateDepthStencilView(inst, &dx12Desc, dsvHandle);
        const HRESULT hr = Device->GetDeviceRemovedReason();

        if(SUCCEEDED(hr))
        {
            const D3D12RHIDepthStencilView view{desc, DSVDescriptorHeap.get(), dsvOffset};
            resource.SetDSV(view);
        }
        else
        {
            TAssertf(false, "Fail to create depth stencil view");
        }
    }
    
    RHIVertexBufferRef D3D12DynamicRHI::RHICreateVertexBuffer(uint32 size,  EResourceUsageFlags usage, void *resourceData)
    {
        ID3D12Resource* vertexBuffer;
        const D3D12_HEAP_PROPERTIES heapType = {
            (EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT),
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };

        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &desc,
                                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                                           nullptr,
                                                           IID_PPV_ARGS(&vertexBuffer));
    
        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIVertexBuffer>(RHIResourceDescriptor::Buffer(size), vertexBuffer);
        }
        else
        {
            TAssertf(false, "Fail to create vertex buffer");
            return nullptr;
        }
    }
    
    RHIIndexBufferRef D3D12DynamicRHI::RHICreateIndexBuffer(uint32 size,  EResourceUsageFlags usage, void *resourceData)
    {
        ID3D12Resource* indexBuffer;
        const D3D12_HEAP_PROPERTIES heapType = {
            EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };
        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &desc,
                                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                                           nullptr,
                                                           IID_PPV_ARGS(&indexBuffer));
    
        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIIndexBuffer>(RHIResourceDescriptor::Buffer(size), indexBuffer);
        }
        else
        {
            TAssertf(false, "Fail to create index buffer");
            return nullptr;
        }
    }
    
    RHIStructuredBufferRef D3D12DynamicRHI::RHICreateStructuredBuffer(uint32 size,  EResourceUsageFlags usage, void *resourceData)
    {
        ID3D12Resource* structuredBuffer;
        const D3D12_HEAP_PROPERTIES heapType = {
            EnumHasAnyFlags(usage, EResourceUsageFlags::AnyDynamic) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };
        
        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &desc,
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
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
    
    RHIConstantBufferRef D3D12DynamicRHI::RHICreateConstantBuffer(uint32 size, EResourceUsageFlags usage, void *resourceData)
    {
        ID3D12Resource* constantBuffer;
        constexpr D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};

        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &desc, //todo: D3D12_HEAP_FLAG_NONE
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                                            nullptr,
                                                            IID_PPV_ARGS(&constantBuffer));
    
        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIConstantBuffer>(RHIResourceDescriptor::Buffer(size), constantBuffer);
        }
        else
        {
            TAssertf(false, "Fail to create constant buffer");
            return nullptr;
        }
    }
    
    RHITexture1DRef D3D12DynamicRHI::RHICreateTexture1D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData)
    {
        ID3D12Resource* texture;
        TAssertf(desc.Type == ERHIResourceType::Texture1D, "Type should be Texture1D.");
        TAssertf(desc.Height == 1, "Height should be 1 for Texture1D.");
        TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Texture1D.");
        constexpr D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};
        const D3D12_RESOURCE_DESC d3d12Desc = {
            D3D12_RESOURCE_DIMENSION_TEXTURE1D,
            desc.Alignment,
            desc.Width,
            1,
            1,
            desc.MipLevels,
            ConvertRHIFormatToD3DFormat(desc.Format),
            {1,0},
            D3D12_TEXTURE_LAYOUT_UNKNOWN,
            GetRHIResourceFlags(desc.Flags)
        };
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &d3d12Desc,
                                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                                           nullptr,
                                                           IID_PPV_ARGS(&texture));
    
        if(SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHITexture1D>(desc, texture);
        }
        else
        {
            TAssertf(false, "Fail to create texture1d");
            return nullptr;
        }
    }
    
    RHITexture2DRef D3D12DynamicRHI::RHICreateTexture2D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData)
    {
        ID3D12Resource* texture;
        TAssertf(desc.Type == ERHIResourceType::Texture2D, "Type should be Texture2D.");
        TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Texture12D.");
        constexpr D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};
        const D3D12_RESOURCE_DESC d3d12Desc = {
            D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            desc.Alignment,
            desc.Width,
            desc.Height,
            1,
            desc.MipLevels,
            ConvertRHIFormatToD3DFormat(desc.Format),
            {1,0},
            D3D12_TEXTURE_LAYOUT_UNKNOWN,
            GetRHIResourceFlags(desc.Flags)
        };
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &d3d12Desc,
                                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                                           nullptr,
                                                           IID_PPV_ARGS(&texture));
    
        if(SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHITexture2D>(desc, texture);
        }
        else
        {
            TAssertf(false, "Fail to create texture2d");
            return nullptr;
        }
    }
    
    RHITexture2DArrayRef D3D12DynamicRHI::RHICreateTexture2DArray(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData)
    {
        ID3D12Resource* texture;
        TAssertf(desc.Type == ERHIResourceType::Texture2DArray, "Type should be Texture2DArray.");
        D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};
        D3D12_RESOURCE_DESC d3d12Desc = {
            D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            desc.Alignment,
            desc.Width,
            desc.Height,
            desc.DepthOrArraySize,
            desc.MipLevels,
            ConvertRHIFormatToD3DFormat(desc.Format),
            {1,0},
            D3D12_TEXTURE_LAYOUT_UNKNOWN,
            GetRHIResourceFlags(desc.Flags)
        };
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &d3d12Desc,
                                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                                           nullptr,
                                                           IID_PPV_ARGS(&texture));
    
        if(SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHITexture2DArray>(desc, texture);
        }
        else
        {
            TAssertf(false, "Fail to create texture2DArray");
            return nullptr;
        }
    }
    
    RHITexture3DRef D3D12DynamicRHI::RHICreateTexture3D(const RHIResourceDescriptor& desc, EResourceUsageFlags usage, void *resourceData)
    {
        ID3D12Resource* texture;
        TAssertf(desc.Type == ERHIResourceType::Texture3D, "Type should be Texture3D.");
        constexpr D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};
        const D3D12_RESOURCE_DESC d3d12Desc = {
            D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            desc.Alignment,
            desc.Width,
            desc.Height,
            desc.DepthOrArraySize,
            desc.MipLevels,
            ConvertRHIFormatToD3DFormat(desc.Format),
            {1,0},
            D3D12_TEXTURE_LAYOUT_UNKNOWN,
            GetRHIResourceFlags(desc.Flags)
        };
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &d3d12Desc,
                                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                                           nullptr,
                                                           IID_PPV_ARGS(&texture));
    
        if(SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHITexture3D>(desc, texture);
        }
        else
        {
            TAssertf(false, "Fail to create texture3d");
            return nullptr;
        }
    }

}
