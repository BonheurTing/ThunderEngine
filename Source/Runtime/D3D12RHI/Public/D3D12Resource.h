#pragma once
#include "Assertion.h"
#include "d3d12.h"
#include "UniformBuffer.h"

namespace Thunder
{
    class D3D12DescriptorHeap;
    class D3D12Device : public RHIDevice
    {
    public:
        D3D12Device() = delete;
        D3D12Device(ID3D12Device * InDevice): Device(InDevice) {}
    
    private:
        ComPtr<ID3D12Device> Device;
    };

    // Allocation handle returned by the upload heap allocator.
    // Represents a sub-region within a larger upload buffer (small block)
    // or an entire committed resource (big block).
    struct D3D12ResourceLocation
    {
        ID3D12Resource* Resource = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;
        void* CPUAddress = nullptr;
        uint32 OffsetInResource = 0;
        uint32 AllocatedSize = 0;
        uint32 BucketIndex = UINT32_MAX;
        uint16 PageIndex = 0;

        bool IsValid() const { return Resource != nullptr; }
        void Invalidate() { Resource = nullptr; CPUAddress = nullptr; GPUVirtualAddress = 0; }

        // Transfers the contents of 1 resource location to another, destroying the original but preserving the underlying resource 
        static void TransferOwnership(D3D12ResourceLocation& Destination, D3D12ResourceLocation& Source);
    };
    
    class D3D12RHIVertexBuffer : public RHIVertexBuffer
    {
    public:
        D3D12RHIVertexBuffer() = delete;
        D3D12RHIVertexBuffer(uint32 sizeInBytes, uint32 strideInBytes, RHIResourceDescriptor const& desc, EBufferCreateFlags const& flags, ID3D12Resource* vb) : RHIVertexBuffer(desc, flags), VertexBuffer(vb)
        {
            VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
            VertexBufferView.StrideInBytes = strideInBytes;
            VertexBufferView.SizeInBytes = sizeInBytes;
        }

        void Update() override;

        _NODISCARD_ void* GetResource() const override { return VertexBuffer.Get(); }
        _NODISCARD_ const D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const { return &VertexBufferView; }
    private:
        ComPtr<ID3D12Resource> VertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW VertexBufferView{};
    };

    class D3D12RHIIndexBuffer : public RHIIndexBuffer
    {
    public:
        D3D12RHIIndexBuffer() = delete;
        D3D12RHIIndexBuffer(ERHIIndexBufferType type, RHIResourceDescriptor const& desc, EBufferCreateFlags const& flags, ID3D12Resource* ib) : RHIIndexBuffer(desc, flags), IndexBuffer(ib)
        {
            IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
            IndexBufferView.Format = type == ERHIIndexBufferType::Uint16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            IndexBufferView.SizeInBytes = static_cast<UINT>(desc.Width);
        }

        void Update() override;
    
        _NODISCARD_ void* GetResource() const override { return IndexBuffer.Get(); }
        _NODISCARD_ const D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const { return &IndexBufferView; }
    private:
        ComPtr<ID3D12Resource> IndexBuffer;
        D3D12_INDEX_BUFFER_VIEW IndexBufferView{};
    };
    
    class D3D12RHIStructuredBuffer : public RHIStructuredBuffer
    {
    public:
        D3D12RHIStructuredBuffer() = delete;
        D3D12RHIStructuredBuffer(RHIResourceDescriptor const& desc, ID3D12Resource* sb) : RHIStructuredBuffer(desc), StructuredBuffer(sb) {}
    
        _NODISCARD_ void * GetResource() const override { return StructuredBuffer.Get(); }
    private:
        ComPtr<ID3D12Resource> StructuredBuffer;
    };
    
    class D3D12RHIConstantBuffer : public RHIConstantBuffer
    {
    public:
        D3D12RHIConstantBuffer() = delete;
        D3D12RHIConstantBuffer(RHIResourceDescriptor const& desc, ID3D12Resource* cb) : RHIConstantBuffer(desc), ConstantBuffer(cb) {}
    
        _NODISCARD_ void * GetResource() const override { return ConstantBuffer.Get(); }
    private:
        ComPtr<ID3D12Resource> ConstantBuffer;
    };
    
    class D3D12RHITexture : public RHITexture
    {
    public:
        D3D12RHITexture() = delete;
        D3D12RHITexture(RHIResourceDescriptor const& desc, ETextureCreateFlags const& flags, ID3D12Resource* texture)
        : RHITexture(desc, flags), Texture(texture) {}

        void Update() override;
    
        _NODISCARD_ void * GetResource() const override { return Texture.Get(); }
    private:
        ComPtr<ID3D12Resource> Texture;
    };

    class D3D12UniformBuffer : public RHIUniformBuffer
    {
    public:
        D3D12UniformBuffer(EUniformBufferFlags usage) : RHIUniformBuffer(usage) {}
        D3D12ResourceLocation ResourceLocation;

        _NODISCARD_ void* GetResource() const override
        {
            return ResourceLocation.Resource;
        }

        uint64 GetGpuVirtualAddress() override
        {
            if (ResourceLocation.IsValid())
            {
                return ResourceLocation.GPUVirtualAddress;
            }
            else
            {
                return 0;
            }
        }
    };

    
    D3D12_RESOURCE_FLAGS GetRHIResourceFlags(RHIResourceFlags flags);

    uint32 BytesPerPixelFromDXGIFormat(DXGI_FORMAT format);
}
