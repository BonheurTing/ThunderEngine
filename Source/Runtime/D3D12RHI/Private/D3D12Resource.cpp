#include "D3D12Resource.h"

#include "D3D12CommandContext.h"
#include "D3D12RHI.h"
#include "d3dx12.h"
#include "IDynamicRHI.h"
#include "IRHIModule.h"
#include "RHIHelper.h"

namespace Thunder
{
	void D3D12RHIVertexBuffer::Update(/*RHICommandList* commandList*/)
	{
		ComPtr<ID3D12Resource> uploadBuffer = VertexBuffer;
		const UINT subresourceCount = Desc.DepthOrArraySize * Desc.MipLevels;
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(VertexBuffer.Get(), 0, subresourceCount);
		bool isDynamic = EnumHasAnyFlags(CreateFlags, EBufferCreateFlags::AnyDynamic);
		D3D12DynamicRHI* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
		if (!isDynamic)
		{
			TAssertf(dx12RHI, "fail to get dx12rhi");
			ComPtr<ID3D12Device> d3dDevice = dx12RHI->GetDevice();

			const D3D12_HEAP_PROPERTIES heapType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			ThrowIfFailed(d3dDevice->CreateCommittedResource(
				&heapType,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadBuffer)));
		}

		void* mappedData = nullptr;
		HRESULT hr = uploadBuffer->Map(0, nullptr, &mappedData);
		TAssertf(SUCCEEDED(hr), "Map failed");
		const uint8* srcData = static_cast<const uint8*>(Data->GetData());
		memcpy(mappedData, srcData, uploadBufferSize);
		uploadBuffer->Unmap(0, nullptr);

		if (!isDynamic)
		{
			if (auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCommandContext().Get()))
			{
				dx12Context->GetCommandList()->CopyResource(VertexBuffer.Get(), uploadBuffer.Get());
			}

			dx12RHI->AddReleaseObject(uploadBuffer);
		}
	}

	void D3D12RHIIndexBuffer::Update()
	{
		ComPtr<ID3D12Resource> uploadBuffer = IndexBuffer;
		const UINT subresourceCount = Desc.DepthOrArraySize * Desc.MipLevels;
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(IndexBuffer.Get(), 0, subresourceCount);
		bool isDynamic = EnumHasAnyFlags(CreateFlags, EBufferCreateFlags::AnyDynamic);
		D3D12DynamicRHI* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
		if (!isDynamic)
		{
			TAssertf(dx12RHI, "fail to get dx12rhi");
			ComPtr<ID3D12Device> d3dDevice = dx12RHI->GetDevice();

			const D3D12_HEAP_PROPERTIES heapType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			ThrowIfFailed(d3dDevice->CreateCommittedResource(
				&heapType,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadBuffer)));
		}

		void* mappedData = nullptr;
		HRESULT hr = uploadBuffer->Map(0, nullptr, &mappedData);
		TAssertf(SUCCEEDED(hr), "Map failed");
		const uint8* srcData = static_cast<const uint8*>(Data->GetData());
		memcpy(mappedData, srcData, uploadBufferSize);
		uploadBuffer->Unmap(0, nullptr);

		if (!isDynamic)
		{
			if (auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCommandContext().Get()))
			{
				dx12Context->GetCommandList()->CopyResource(IndexBuffer.Get(), uploadBuffer.Get());
			}

			dx12RHI->AddReleaseObject(uploadBuffer);
		}
	}

	void D3D12RHITexture::Update(/*RHICommandList* commandList*/)
	{
		ComPtr<ID3D12Resource> uploadTexture = Texture;
		bool isDynamic = EnumHasAnyFlags(CreateFlags, ETextureCreateFlags::AnyDynamic);
		D3D12DynamicRHI* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
		if (!isDynamic)
		{
			TAssertf(dx12RHI, "fail to get dx12rhi");
			ComPtr<ID3D12Device> d3dDevice = dx12RHI->GetDevice();

			const UINT subresourceCount = Desc.DepthOrArraySize * Desc.MipLevels;
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(Texture.Get(), 0, subresourceCount);
			const D3D12_HEAP_PROPERTIES heapType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			ThrowIfFailed(d3dDevice->CreateCommittedResource(
				&heapType,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadTexture)));
		}

		uint16 mipLevel = Desc.MipLevels;
		UINT offset = 0;
		for (uint32 array = 0; array < Desc.DepthOrArraySize; ++array)
		{
			for (UINT mipId = 0; mipId < mipLevel; ++mipId)
			{
				UINT8* dstTexture;
				UINT subResource = array * Desc.DepthOrArraySize + mipId;
				CD3DX12_RANGE readRange(0, 0);
				HRESULT hr = uploadTexture->Map(subResource, &readRange, reinterpret_cast<void**>(&dstTexture));
				TAssertf(SUCCEEDED(hr), "Map failed");

				uint8* srcData = static_cast<uint8*>(Data->Data) + offset;
				size_t mipSize = static_cast<size_t>(Desc.Width) / pow(2, mipLevel) * (Desc.Height > 1 ? Desc.Height / pow(2, mipLevel) : 1);
				memcpy(dstTexture, srcData, mipSize);
				//MemcpySubresource()

				uploadTexture->Unmap(subResource, nullptr);
				offset += mipSize;
			}
		}

		if (!isDynamic)
		{
			for (uint32 array = 0; array < Desc.DepthOrArraySize; ++array)
			{
				for (uint32 mipId = 0; mipId < Desc.MipLevels; ++mipId)
				{
					UINT subResource = array * Desc.DepthOrArraySize + mipId;
					const CD3DX12_TEXTURE_COPY_LOCATION dstLocation(Texture.Get(), subResource);
					const CD3DX12_TEXTURE_COPY_LOCATION srcLocation(uploadTexture.Get(), subResource);

					if (auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCommandContext().Get()))
					{
						dx12Context->GetCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
					}
				}
			}

			dx12RHI->AddReleaseObject(uploadTexture);
		}
	}

	D3D12_RESOURCE_FLAGS GetRHIResourceFlags(RHIResourceFlags flags)
	{
		D3D12_RESOURCE_FLAGS outFlags = D3D12_RESOURCE_FLAG_NONE;
		if(flags.DenySRV)
		{
			outFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
		if(flags.NeedUAV)
		{
			outFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if(flags.NeedDSV)
		{
			outFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if(flags.NeedRTV)
		{
			outFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		return outFlags;
	}
}
