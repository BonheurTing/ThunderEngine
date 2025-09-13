#pragma once

#include "d3d11.h"
#include "RHIResource.h"

namespace Thunder
{
	class D3D11Device : public RHIDevice
    {
    public:
    	D3D11Device() = delete;
    	D3D11Device(ID3D11Device * InDevice): Device(InDevice) {}
    
    private:
	    ComPtr<ID3D11Device> Device;
    };

    class D3D11RHIVertexDeclaration : public RHIVertexDeclarationDescriptor
    {
    public:
        D3D11RHIVertexDeclaration() = delete;
        D3D11RHIVertexDeclaration(TArray<RHIVertexElement> const& inElements, ID3D11InputLayout * inInputLayout)
        : RHIVertexDeclarationDescriptor(inElements), InputLayout(inInputLayout) {}
    private:
        ComPtr<ID3D11InputLayout> InputLayout;
    };
    
    class D3D11RHIVertexBuffer : public RHIVertexBuffer
    {
    public:
        D3D11RHIVertexBuffer() = delete;
        D3D11RHIVertexBuffer(RHIResourceDescriptor const& desc, ID3D11Resource* vb) : RHIVertexBuffer(desc), VertexBuffer(vb) {}

        _NODISCARD_ void * GetResource() const override { return VertexBuffer.Get(); }
    private:
        ComPtr<ID3D11Resource> VertexBuffer;
    };
    
    class D3D11RHIIndexBuffer : public RHIIndexBuffer
    {
    public:
        D3D11RHIIndexBuffer() = delete;
        D3D11RHIIndexBuffer(RHIResourceDescriptor const& desc, ID3D11Resource* vb) : RHIIndexBuffer(desc), IndexBuffer(vb) {}
    
        _NODISCARD_ void * GetResource() const override { return IndexBuffer.Get(); }
    private:
        ComPtr<ID3D11Resource> IndexBuffer;
    };
    
    class D3D11RHIStructuredBuffer : public RHIStructuredBuffer
    {
    public:
        D3D11RHIStructuredBuffer() = delete;
        D3D11RHIStructuredBuffer(RHIResourceDescriptor const& desc, ID3D11Resource* sb) : RHIStructuredBuffer(desc), StructuredBuffer(sb) {}
    
        _NODISCARD_ void * GetResource() const override { return StructuredBuffer.Get(); }
    private:
        ComPtr<ID3D11Resource> StructuredBuffer;
    };
    
    class D3D11RHIConstantBuffer : public RHIConstantBuffer
    {
    public:
        D3D11RHIConstantBuffer() = delete;
        D3D11RHIConstantBuffer(RHIResourceDescriptor const& desc, ID3D11Resource* cb) : RHIConstantBuffer(desc), ConstantBuffer(cb) {}
    
        _NODISCARD_ void * GetResource() const override { return ConstantBuffer.Get(); }
    private:
        ComPtr<ID3D11Resource> ConstantBuffer;
    };
    
    class D3D11RHITexture : public RHITexture
    {
    public:
        D3D11RHITexture() = delete;
        D3D11RHITexture(RHIResourceDescriptor const& desc, ID3D11Resource* texture) : RHITexture(desc), Texture(texture) {}
    
        _NODISCARD_ void * GetResource() const override { return Texture.Get(); }
    private:
        ComPtr<ID3D11Resource> Texture;
    };

    UINT GetRHIResourceBindFlags(RHIResourceFlags flags);
}
