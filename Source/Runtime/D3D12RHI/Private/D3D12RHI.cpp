#include "D3D12RHI.h"
#include "D3D12Resource.h"
#include "D3D12DescriptorHeap.h"
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

    RHIConstantBufferViewRef D3D12DynamicRHI::RHICreateConstantBufferView(const RHIViewDescriptor& desc)
    {
        return nullptr;
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
        auto resourceDesc = resource.GetResourceDescriptor();
        switch (resourceDesc->Type)
        {
        case ERHIResourceType::Buffer:
            dx12Desc.Buffer.NumElements = static_cast<UINT>(resourceDesc->Width);
            break;
        case ERHIResourceType::Texture1D:
            dx12Desc.Texture1D.MipLevels = resourceDesc->MipLevels;
            break;
        case ERHIResourceType::Texture2D:
            dx12Desc.Texture2D.MipLevels = resourceDesc->MipLevels;
            break;
        case ERHIResourceType::Texture2DArray:
            dx12Desc.Texture2DArray.MipLevels = resourceDesc->MipLevels;
            dx12Desc.Texture2DArray.ArraySize = resourceDesc->DepthOrArraySize;
            break;
        case ERHIResourceType::Texture3D:
            dx12Desc.Texture3D.MipLevels = resourceDesc->MipLevels;
            break;
        case ERHIResourceType::Unknown: break;
        }
        const auto inst = static_cast<ID3D12Resource*>( resource.GetResource() );
        Device->CreateShaderResourceView(inst, &dx12Desc, srvHandle);

        const D3D12RHIShaderResourceView view{desc, CommonDescriptorHeap.get(), srvOffset};
        resource.SetSRV(view);
    }

    RHIUnorderedAccessViewRef D3D12DynamicRHI::RHICreateUnorderedAccessView(const RHIViewDescriptor& desc)
    {
        return nullptr;
    }

    RHIRenderTargetViewRef D3D12DynamicRHI::RHICreateRenderTargetView(const RHIViewDescriptor& desc)
    {
        return nullptr;
    }

    RHIDepthStencilViewRef D3D12DynamicRHI::RHICreateDepthStencilView(const RHIViewDescriptor& desc)
    {
        return nullptr;
    }
    
    RHIVertexBufferRef D3D12DynamicRHI::RHICreateVertexBuffer(const RHIResourceDescriptor& desc)
    {
        ID3D12Resource* vertexBuffer;
        TAssertf(desc.Type == ERHIResourceType::Buffer, "Resource type should be buffer.");
        TAssertf(desc.Height == 1, "Height should be 1 for Vertex buffer.");
        TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Vertex buffer.");
        TAssertf(desc.MipLevels == 1, "Mip level count should be 1 for Vertex buffer.");
        constexpr D3D12_HEAP_PROPERTIES heapType = {
            D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };
        const D3D12_RESOURCE_DESC d3d12Desc = {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            desc.Alignment,
            desc.Width,
            1,
            1,
            1,
            ConvertRHIFormatToD3DFormat(desc.Format),
            {1, 0},
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            GetRHIResourceFlags(desc.Flags)
        };
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &d3d12Desc,
                                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                                           nullptr,
                                                           IID_PPV_ARGS(&vertexBuffer));
    
        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIVertexBuffer>(desc, vertexBuffer);
        }
        else
        {
            LOG("Fail to Create VertexBuffer (dx12)");
            return nullptr;
        }
    }
    
    RHIIndexBufferRef D3D12DynamicRHI::RHICreateIndexBuffer(const RHIResourceDescriptor& desc)
    {
        ID3D12Resource* indexBuffer;
        TAssertf(desc.Type == ERHIResourceType::Buffer, "Resource type should be IndexBuffer.");
        TAssertf(desc.Height == 1, "Height should be 1 for IndexBuffer.");
        TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for IndexBuffer.");
        TAssertf(desc.MipLevels == 1, "Mip level count should be 1 for IndexBuffer.");
        constexpr D3D12_HEAP_PROPERTIES heapType = {
            D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };
        const D3D12_RESOURCE_DESC d3d12Desc = {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            desc.Alignment,
            desc.Width,
            1,
            1,
            1,
            ConvertRHIFormatToD3DFormat(desc.Format),
            {1, 0},
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            GetRHIResourceFlags(desc.Flags)
        };
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                           D3D12_HEAP_FLAG_NONE,
                                                           &d3d12Desc,
                                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                                           nullptr,
                                                           IID_PPV_ARGS(&indexBuffer));
    
        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIIndexBuffer>(desc, indexBuffer);
        }
        else
        {
            LOG("Fail to Create IndexBuffer (dx12)");
            return nullptr;
        }
    }
    
    RHIStructuredBufferRef D3D12DynamicRHI::RHICreateStructuredBuffer(const RHIResourceDescriptor& desc)
    {
        ID3D12Resource* structuredBuffer;
        TAssertf(desc.Type == ERHIResourceType::Buffer, "Resource type should be structured buffer.");
        TAssertf(desc.Height == 1, "Height should be 1 for structured Buffer.");
        TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for structured Buffer.");
        TAssertf(desc.MipLevels == 1, "Mip level count should be 1 for structured buffer.");
        constexpr D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};
        const D3D12_RESOURCE_DESC d3d12Desc = {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            desc.Alignment,
            desc.Width,
            desc.Height,
            desc.DepthOrArraySize,
            1,
            ConvertRHIFormatToD3DFormat(desc.Format),
            {1,0},
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            GetRHIResourceFlags(desc.Flags)
        };
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &d3d12Desc,
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                                            nullptr,
                                                            IID_PPV_ARGS(&structuredBuffer));
    
        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIStructuredBuffer>(desc, structuredBuffer);
        }
        else
        {
            LOG("Fail to Create Structured Buffer (dx12)");
            return nullptr;
        }
    }
    
    RHIConstantBufferRef D3D12DynamicRHI::RHICreateConstantBuffer(const RHIResourceDescriptor& desc)
    {
        ID3D12Resource* constantBuffer;
        TAssertf(desc.Type == ERHIResourceType::Buffer, "Resource type should be constant buffer.");
        TAssertf(desc.Height == 1, "Height should be 1 for constant Buffer.");
        TAssertf(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for constant Buffer.");
        TAssertf(desc.MipLevels == 1, "Mip level count should be 1 for constant buffer.");
        constexpr D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};
        const D3D12_RESOURCE_DESC d3d12Desc = {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            desc.Alignment,
            desc.Width,
            1,
            1,
            1,
            ConvertRHIFormatToD3DFormat(desc.Format),
            {1,0},
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            GetRHIResourceFlags(desc.Flags)
        };
        const HRESULT hr = Device->CreateCommittedResource(&heapType,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &d3d12Desc,
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                                            nullptr,
                                                            IID_PPV_ARGS(&constantBuffer));
    
        if (SUCCEEDED(hr))
        {
            return MakeRefCount<D3D12RHIConstantBuffer>(desc, constantBuffer);
        }
        else
        {
            LOG("Fail to Create Constant Buffer (dx12)");
            return nullptr;
        }
    }
    
    RHITexture1DRef D3D12DynamicRHI::RHICreateTexture1D(const RHIResourceDescriptor& desc)
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
            LOG("Fail to Create Texture1D (dx12)");
            return nullptr;
        }
    }
    
    RHITexture2DRef D3D12DynamicRHI::RHICreateTexture2D(const RHIResourceDescriptor& desc)
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
            LOG("Fail to Create Texture2D (dx12)");
            return nullptr;
        }
    }
    
    RHITexture2DArrayRef D3D12DynamicRHI::RHICreateTexture2DArray(const RHIResourceDescriptor& desc)
    {
        ID3D12Resource* texture;
        TAssertf(desc.Type == ERHIResourceType::Texture2DArray, "Type should be Texture2DArray.");
        D3D12_HEAP_PROPERTIES heapType = {D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1};
        D3D12_RESOURCE_DESC d3d12Desc = {
            D3D12_RESOURCE_DIMENSION_TEXTURE3D,
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
            LOG("Fail to Create Texture2DArray (dx12)");
            return nullptr;
        }
    }
    
    RHITexture3DRef D3D12DynamicRHI::RHICreateTexture3D(const RHIResourceDescriptor& desc)
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
            LOG("Fail to Create Texture3D (dx12)");
            return nullptr;
        }
    }

}
