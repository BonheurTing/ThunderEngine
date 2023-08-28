#pragma once
#include "d3d12.h"
#include "RHIResource.h"

namespace Thunder
{
    class D3D12Device : public RHIDevice
    {
    public:
        D3D12Device() = delete;
        ~D3D12Device() = default;
        D3D12Device(ID3D12Device * InDevice): Device(InDevice) {}
    
    private:
        ComPtr<ID3D12Device> Device;
    };
    
    class D3D12RHIVertexBuffer : public RHIVertexBuffer
    {
    public:
        D3D12RHIVertexBuffer() = delete;
        D3D12RHIVertexBuffer(RHIResourceDescriptor const& desc, ID3D12Resource* vb) : RHIVertexBuffer(desc), VertexBuffer(vb) {}
        void* GetShaderResourceView() override;
    
    private:
        ComPtr<ID3D12Resource> VertexBuffer;
    };
    
    class D3D12RHIIndexBuffer : public RHIIndexBuffer
    {
    public:
        D3D12RHIIndexBuffer() = delete;
        D3D12RHIIndexBuffer(RHIResourceDescriptor const& desc, ID3D12Resource* vb) : RHIIndexBuffer(desc), IndexBuffer(vb) {}
    
    private:
        ComPtr<ID3D12Resource> IndexBuffer;
    };
    
    class D3D12RHIStructuredBuffer : public RHIStructuredBuffer
    {
    public:
        D3D12RHIStructuredBuffer() = delete;
        D3D12RHIStructuredBuffer(RHIResourceDescriptor const& desc, ID3D12Resource* sb) : RHIStructuredBuffer(desc), StructuredBuffer(sb) {}
    
    private:
        ComPtr<ID3D12Resource> StructuredBuffer;
    };
    
    class D3D12RHIConstantBuffer : public RHIConstantBuffer
    {
    public:
        D3D12RHIConstantBuffer() = delete;
        D3D12RHIConstantBuffer(RHIResourceDescriptor const& desc, ID3D12Resource* cb) : RHIConstantBuffer(desc), ConstantBuffer(cb) {}
    
    private:
        ComPtr<ID3D12Resource> ConstantBuffer;
    };
    
    class D3D12RHITexture1D : public RHITexture1D
    {
    public:
        D3D12RHITexture1D() = delete;
        D3D12RHITexture1D(RHIResourceDescriptor const& desc, ID3D12Resource* texture) : RHITexture1D(desc), Texture1D(texture) {}
    
    private:
        ComPtr<ID3D12Resource> Texture1D;
    };
    
    class D3D12RHITexture2D : public RHITexture2D
    {
    public:
        D3D12RHITexture2D() = delete;
        D3D12RHITexture2D(RHIResourceDescriptor const& desc, ID3D12Resource* texture) : RHITexture2D(desc), Texture2D(texture) {}
    
    private:
        ComPtr<ID3D12Resource> Texture2D;
    };
    
    class D3D12RHITexture2DArray : public RHITexture2DArray
    {
    public:
        D3D12RHITexture2DArray() = delete;
        D3D12RHITexture2DArray(RHIResourceDescriptor const& desc, ID3D12Resource* texture) : RHITexture2DArray(desc), Texture2DArray(texture) {}
    
    private:
        ComPtr<ID3D12Resource> Texture2DArray;
    };
    
    class D3D12RHITexture3D : public RHITexture3D
    {
    public:
        D3D12RHITexture3D() = delete;
        D3D12RHITexture3D(RHIResourceDescriptor const& desc, ID3D12Resource* texture) : RHITexture3D(desc), Texture3D(texture) {}
    
    private:
        ComPtr<ID3D12Resource> Texture3D;
    };
}
