#include "D3D12Resource.h"

#include "D3D12CommandContext.h"
#include "D3D12RHI.h"
#include "d3dx12.h"
#include "IDynamicRHI.h"
#include "IRHIModule.h"
#include "RHIHelper.h"

namespace Thunder
{
	void D3D12RHITexture::Update(/*RHICommandList* commandList*/)
	{
		ComPtr<ID3D12Resource> textureUploadHeap = Texture;
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
				IID_PPV_ARGS(&textureUploadHeap)));
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
				HRESULT hr = textureUploadHeap->Map(subResource, &readRange, reinterpret_cast<void**>(&dstTexture));
				TAssertf(SUCCEEDED(hr), "Map failed");

				uint8* srcData = static_cast<uint8*>(Data->Data) + offset;
				size_t mipSize = static_cast<size_t>(Desc.Width) / pow(2, mipLevel) * (Desc.Height > 1 ? Desc.Height / pow(2, mipLevel) : 1);
				memcpy(dstTexture, srcData, mipSize);
				//MemcpySubresource()

				textureUploadHeap->Unmap(subResource, nullptr);
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
					const CD3DX12_TEXTURE_COPY_LOCATION srcLocation(textureUploadHeap.Get(), subResource);

					if (auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCommandContext().Get()))
					{
						dx12Context->GetCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
					}
				}
			}

			dx12RHI->AddReleaseObject(textureUploadHeap);
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
