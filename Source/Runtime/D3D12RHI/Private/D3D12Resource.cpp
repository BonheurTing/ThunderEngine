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
			if (auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCopyCommandContext()))
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
			if (auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCopyCommandContext()))
			{
				dx12Context->GetCommandList()->CopyResource(IndexBuffer.Get(), uploadBuffer.Get());
			}

			dx12RHI->AddReleaseObject(uploadBuffer);
		}
	}

	void D3D12RHITexture::Update(/*RHICommandList* commandList*/)
	{
		bool isDynamic = EnumHasAnyFlags(CreateFlags, ETextureCreateFlags::AnyDynamic);
	    D3D12DynamicRHI* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
	    TAssertf(dx12RHI, "fail to get dx12rhi");
	    ComPtr<ID3D12Device> d3dDevice = dx12RHI->GetDevice();
		D3D12_RESOURCE_DESC textureDesc = Texture->GetDesc();

	    if (isDynamic)
	    {
	        // 动态纹理可直接 Map 修改，不赘述
	    	TAssertf(false, "todo");
	        return;
	    }

	    // 1. Calculate the size required for the Upload Buffer
	    const UINT subresourceCount = Desc.DepthOrArraySize * Desc.MipLevels;
	    UINT64 uploadBufferSize = GetRequiredIntermediateSize(Texture.Get(), 0, subresourceCount);

	    // 2. create Upload Buffer
	    ComPtr<ID3D12Resource> uploadBuffer;
		const D3D12_HEAP_PROPERTIES heapType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	    ThrowIfFailed(d3dDevice->CreateCommittedResource(
	        &heapType,
	        D3D12_HEAP_FLAG_NONE,
	        &bufferDesc,
	        D3D12_RESOURCE_STATE_GENERIC_READ,
	        nullptr,
	        IID_PPV_ARGS(&uploadBuffer)));

	    // 3. copy data to Upload Buffer
	    uint8_t* mappedData = nullptr;
	    ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

	    UINT64 srcOffset = 0; // Source data offset
	    for (UINT arraySlice = 0; arraySlice < textureDesc.DepthOrArraySize; ++arraySlice)
	    {
	        for (UINT mip = 0; mip < textureDesc.MipLevels; ++mip)
	        {
	            UINT subresourceIndex = D3D12CalcSubresource(mip, arraySlice, 0, textureDesc.MipLevels, textureDesc.DepthOrArraySize);

	            // Get footprint (Pitch, Offset, size, etc.)
	            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	            UINT numRows = 0;
	            UINT64 rowSizeInBytes = 0;
	            UINT64 totalBytes = 0;
	            d3dDevice->GetCopyableFootprints(
	                &textureDesc,  // Use texture desc instead of buffer desc
	                subresourceIndex,
	                1,
	                0,
	                &footprint,
	                &numRows,
	                &rowSizeInBytes,
	                &totalBytes);

	            // Copy data row by row (considering RowPitch alignment)
	            const uint8* srcData = static_cast<const uint8*>(Data->Data) + srcOffset;
	            uint8* dstData = mappedData + footprint.Offset;

	            uint32 bytesPerPixel = BytesPerPixelFromDXGIFormat(textureDesc.Format);
	            uint32 rowPitch = footprint.Footprint.RowPitch;
	            uint32 rowBytes = footprint.Footprint.Width * bytesPerPixel;

	            for (uint32 z = 0; z < footprint.Footprint.Depth; ++z)
	            {
	                const uint8* srcSlice = srcData + z * rowBytes * numRows;
	                uint8* dstSlice = dstData + z * rowPitch * numRows;

	                for (uint32 y = 0; y < numRows; ++y)
	                {
	                    memcpy(dstSlice + y * rowPitch, srcSlice + y * rowBytes, rowBytes);
	                }
	            }

	            // Update source offset
	            srcOffset += rowBytes * numRows * footprint.Footprint.Depth;
	        }
	    }

	    uploadBuffer->Unmap(0, nullptr);

	    // 4. 生成 CopyTextureRegion 命令
	    if (auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCopyCommandContext()))
	    {
	        for (UINT arraySlice = 0; arraySlice < textureDesc.DepthOrArraySize; ++arraySlice)
	        {
	            for (UINT mip = 0; mip < textureDesc.MipLevels; ++mip)
	            {
	                UINT subresourceIndex = D3D12CalcSubresource(mip, arraySlice, 0, textureDesc.MipLevels, textureDesc.DepthOrArraySize);

	                D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	                UINT numRows = 0;
	                UINT64 rowSizeInBytes = 0;
	                UINT64 totalBytes = 0;
	                d3dDevice->GetCopyableFootprints(
	                    &textureDesc,  // Use texture desc instead of buffer desc
	                    subresourceIndex,
	                    1,
	                    0,
	                    &footprint,
	                    &numRows,
	                    &rowSizeInBytes,
	                    &totalBytes);

	                CD3DX12_TEXTURE_COPY_LOCATION dstLocation(Texture.Get(), subresourceIndex);

	                D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
	                srcLocation.pResource = uploadBuffer.Get();
	                srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	                srcLocation.PlacedFootprint = footprint;

	                dx12Context->GetCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
	            }
	        }
	    }

	    dx12RHI->AddReleaseObject(uploadBuffer);
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

	uint32 BytesPerPixelFromDXGIFormat(DXGI_FORMAT format)
	{
		switch (format)
		{
			// 8-bit formats
			case DXGI_FORMAT_R8_TYPELESS:
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT_R8_SINT:
			case DXGI_FORMAT_A8_UNORM:
				return 1;

			// 16-bit formats
			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT_R16_SINT:
			case DXGI_FORMAT_R8G8_TYPELESS:
			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT_R8G8_SINT:
			case DXGI_FORMAT_B5G6R5_UNORM:
			case DXGI_FORMAT_B5G5R5A1_UNORM:
				return 2;

			// 32-bit formats
			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			case DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT_R16G16_SINT:
			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT_R8G8B8A8_SINT:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT_R10G10B10A2_TYPELESS:
			case DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT_R11G11B10_FLOAT:
			case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
				return 4;

			// 64-bit formats
			case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT_R16G16B16A16_SINT:
			case DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT_R32G32_SINT:
			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return 8;

			// 96-bit formats
			case DXGI_FORMAT_R32G32B32_TYPELESS:
			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT_R32G32B32_SINT:
				return 12;

			// 128-bit formats
			case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32A32_SINT:
				return 16;

			// Compressed formats (block-compressed)
			// BC1/DXT1 (4 bits per pixel)
			case DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
			case DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM:
				return 8; // 8 bytes per 4x4 block

			// BC2/DXT3, BC3/DXT5, BC5, BC6, BC7 (8 bits per pixel)
			case DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
			case DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
			case DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC5_SNORM:
			case DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT_BC6H_UF16:
			case DXGI_FORMAT_BC6H_SF16:
			case DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				return 16; // 16 bytes per 4x4 block

			// Unknown or unsupported format
			default:
				TAssertf(false, "Unsupported DXGI_FORMAT");
				return 0;
		}
	}
}
