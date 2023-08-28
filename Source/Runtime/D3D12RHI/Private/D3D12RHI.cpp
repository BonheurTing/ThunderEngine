#include "D3D12RHI.h"
#include "D3D12Resource.h"
#include "RHIHelper.h"
#include "Assertion.h"
#include <dxgi1_4.h>

namespace Thunder
{
    D3D12DynamicRHI::D3D12DynamicRHI() {}
    
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
    
        if(SUCCEEDED(hr))
        {
            return RefCountPtr<D3D12Device>{ new D3D12Device{Device.Get()} };
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
    
    RHIPixelShaderRef D3D12DynamicRHI::RHICreatePixelShader()
    {
        return nullptr;
    }
    
    RHIVertexShaderRef D3D12DynamicRHI::RHICreateVertexShader()
    {
        return nullptr;
    }
    
    RHIVertexBufferRef D3D12DynamicRHI::RHICreateVertexBuffer(const RHIResourceDescriptor& desc)
    {
        ID3D12Resource* vertexBuffer;
        TAssert(desc.Type == ERHIResourceType::Buffer, "Resource type should be buffer.");
        TAssert(desc.Height == 1, "Height should be 1 for Vertex buffer.");
        TAssert(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Vertex buffer.");
        TAssert(desc.MipLevels == 1, "Mip level count should be 1 for Vertex buffer.");
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
            return RefCountPtr<D3D12RHIVertexBuffer>{new D3D12RHIVertexBuffer(desc, vertexBuffer)};
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
        TAssert(desc.Type == ERHIResourceType::Buffer, "Resource type should be IndexBuffer.");
        TAssert(desc.Height == 1, "Height should be 1 for IndexBuffer.");
        TAssert(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for IndexBuffer.");
        TAssert(desc.MipLevels == 1, "Mip level count should be 1 for IndexBuffer.");
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
            return RefCountPtr<D3D12RHIIndexBuffer>{new D3D12RHIIndexBuffer(desc, indexBuffer)};
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
        TAssert(desc.Type == ERHIResourceType::Buffer, "Resource type should be structured buffer.");
        TAssert(desc.Height == 1, "Height should be 1 for structured Buffer.");
        TAssert(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for structured Buffer.");
        TAssert(desc.MipLevels == 1, "Mip level count should be 1 for structured buffer.");
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
            return RefCountPtr<D3D12RHIStructuredBuffer>{new D3D12RHIStructuredBuffer(desc, structuredBuffer)};
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
        TAssert(desc.Type == ERHIResourceType::Buffer, "Resource type should be constant buffer.");
        TAssert(desc.Height == 1, "Height should be 1 for constant Buffer.");
        TAssert(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for constant Buffer.");
        TAssert(desc.MipLevels == 1, "Mip level count should be 1 for constant buffer.");
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
            return RefCountPtr<D3D12RHIConstantBuffer>{new D3D12RHIConstantBuffer(desc, constantBuffer)};
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
        TAssert(desc.Type == ERHIResourceType::Texture1D, "Type should be Texture1D.");
        TAssert(desc.Height == 1, "Height should be 1 for Texture1D.");
        TAssert(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Texture1D.");
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
            return RefCountPtr<D3D12RHITexture1D>{new D3D12RHITexture1D(desc, texture)};
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
        TAssert(desc.Type == ERHIResourceType::Texture2D, "Type should be Texture2D.");
        TAssert(desc.DepthOrArraySize == 1, "DepthOrArraySize should be 1 for Texture12D.");
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
            return RefCountPtr<D3D12RHITexture2D>{new D3D12RHITexture2D(desc, texture)};
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
        TAssert(desc.Type == ERHIResourceType::Texture2DArray, "Type should be Texture2DArray.");
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
            return RefCountPtr<D3D12RHITexture2DArray>{new D3D12RHITexture2DArray(desc, texture)};
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
        TAssert(desc.Type == ERHIResourceType::Texture3D, "Type should be Texture3D.");
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
            return RefCountPtr<D3D12RHITexture3D>{new D3D12RHITexture3D(desc, texture)};
        }
        else
        {
            LOG("Fail to Create Texture3D (dx12)");
            return nullptr;
        }
    }

}
