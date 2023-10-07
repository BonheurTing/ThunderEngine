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

	
    //class D3D11Desc : public 
    
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
    
    class D3D11RHITexture1D : public RHITexture1D
    {
    public:
        D3D11RHITexture1D() = delete;
        D3D11RHITexture1D(RHIResourceDescriptor const& desc, ID3D11Texture1D* texture) : RHITexture1D(desc), Texture1D(texture) {}
    
        _NODISCARD_ void * GetResource() const override { return Texture1D.Get(); }
    private:
        ComPtr<ID3D11Texture1D> Texture1D;
    };
    
    class D3D11RHITexture2D : public RHITexture2D
    {
    public:
        D3D11RHITexture2D() = delete;
        D3D11RHITexture2D(RHIResourceDescriptor const& desc, ID3D11Texture2D* texture) : RHITexture2D(desc), Texture2D(texture) {}
    
        _NODISCARD_ void * GetResource() const override { return Texture2D.Get(); }
    private:
        ComPtr<ID3D11Texture2D> Texture2D;
    };
    
    class D3D11RHITexture2DArray : public RHITexture2DArray
    {
    public:
        D3D11RHITexture2DArray() = delete;
        D3D11RHITexture2DArray(RHIResourceDescriptor const& desc, ID3D11Texture2D* texture) : RHITexture2DArray(desc), Texture2DArray(texture) {}
    
        _NODISCARD_ void * GetResource() const override { return Texture2DArray.Get(); }
    private:
        ComPtr<ID3D11Texture2D> Texture2DArray;
    };
    
    class D3D11RHITexture3D : public RHITexture3D
    {
    public:
        D3D11RHITexture3D() = delete;
        D3D11RHITexture3D(RHIResourceDescriptor const& desc, ID3D11Texture3D* texture) : RHITexture3D(desc), Texture3D(texture) {}
    
        _NODISCARD_ void * GetResource() const override { return Texture3D.Get(); }
    private:
        ComPtr<ID3D11Texture3D> Texture3D;
    };

    UINT GetRHIResourceBindFlags(RHIResourceFlags flags);
}
