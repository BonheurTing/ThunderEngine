#include "D3D12Resource.h"

#include "D3D12CommandContext.h"
#include "D3D12RHI.h"
#include "d3dx12.h"
#include "IDynamicRHI.h"
#include "IRHIModule.h"
#include "RHIHelper.h"

namespace Thunder
{
	void D3D12ResourceLocation::TransferOwnership(D3D12ResourceLocation& Destination, D3D12ResourceLocation& Source)
	{
		memmove(&Destination, &Source, sizeof(D3D12ResourceLocation));
	}

	void D3D12RHIVertexBuffer::Update(/*RHICommandList* commandList*/)
	{
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(VertexBuffer.Get(), 0, 1);
		bool isDynamic = EnumHasAnyFlags(CreateFlags, EBufferCreateFlags::AnyDynamic);
		D3D12DynamicRHI* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
		TAssertf(dx12RHI, "fail to get dx12rhi");

		if (isDynamic)
		{
			// Dynamic buffer lives on upload heap, map directly.
			void* mappedData = nullptr;
			HRESULT hr = VertexBuffer->Map(0, nullptr, &mappedData);
			if (hr != S_OK) [[unlikely]]
			{
				TAssertf(false, "D3D12RHIVertexBuffer::Update: Map failed (HRESULT: 0x%08X).", static_cast<uint32>(hr));
				return;
			}
			memcpy(mappedData, Data, uploadBufferSize);
			VertexBuffer->Unmap(0, nullptr);
			return;
		}

		// Static buffer: create a temporary upload heap buffer, memcpy into it,
		// then CopyBufferRegion to the default heap buffer with the correct barriers.
		ComPtr<ID3D12Device> d3dDevice = dx12RHI->GetDevice();

		ComPtr<ID3D12Resource> uploadBuffer;
		const D3D12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		HRESULT hr = d3dDevice->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer));
		if (hr != S_OK) [[unlikely]]
		{
			TAssertf(false, "D3D12RHIVertexBuffer::Update: Failed to create upload buffer (HRESULT: 0x%08X).", static_cast<uint32>(hr));
			return;
		}

		// Map upload buffer and copy CPU data into it.
		void* mappedData = nullptr;
		const D3D12_RANGE readRange = { 0, 0 };
		hr = uploadBuffer->Map(0, &readRange, &mappedData);
		if (hr != S_OK) [[unlikely]]
		{
			TAssertf(false, "D3D12RHIVertexBuffer::Update: Failed to map upload buffer (HRESULT: 0x%08X).", static_cast<uint32>(hr));
			return;
		}
		memcpy(mappedData, Data, uploadBufferSize);
		uploadBuffer->Unmap(0, nullptr);

		auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCopyCommandContext_RHI());
		if (!dx12Context) [[unlikely]]
		{
			TAssertf(false, "D3D12RHIVertexBuffer::Update: Failed to get copy command context.");
			return;
		}

		auto* cmdList = dx12Context->GetCommandList().Get();

		// Barrier: COMMON -> COPY_DEST
		D3D12_RESOURCE_STATES oldState = static_cast<D3D12_RESOURCE_STATES>(GetState_RenderThread());
		if (oldState != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			CD3DX12_RESOURCE_BARRIER toCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
				VertexBuffer.Get(), oldState, D3D12_RESOURCE_STATE_COPY_DEST);
			cmdList->ResourceBarrier(1, &toCopyDest);
		}

		cmdList->CopyBufferRegion(VertexBuffer.Get(), 0, uploadBuffer.Get(), 0, uploadBufferSize);

		// Barrier: COPY_DEST -> VERTEX_AND_CONSTANT_BUFFER
		CD3DX12_RESOURCE_BARRIER toVBState = CD3DX12_RESOURCE_BARRIER::Transition(
			VertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		cmdList->ResourceBarrier(1, &toVBState);
		SetState_RenderThread(static_cast<ERHIResourceState>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		dx12RHI->AddReleaseObject(uploadBuffer);
	}

	void D3D12RHIIndexBuffer::Update()
	{
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(IndexBuffer.Get(), 0, 1);
		bool isDynamic = EnumHasAnyFlags(CreateFlags, EBufferCreateFlags::AnyDynamic);
		D3D12DynamicRHI* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
		TAssertf(dx12RHI, "fail to get dx12rhi");

		if (isDynamic)
		{
			// Dynamic buffer lives on upload heap, map directly.
			void* mappedData = nullptr;
			HRESULT hr = IndexBuffer->Map(0, nullptr, &mappedData);
			if (hr != S_OK) [[unlikely]]
			{
				TAssertf(false, "D3D12RHIIndexBuffer::Update: Map failed (HRESULT: 0x%08X).", static_cast<uint32>(hr));
				return;
			}
			memcpy(mappedData, Data, uploadBufferSize);
			IndexBuffer->Unmap(0, nullptr);
			return;
		}

		// Static buffer: create a temporary upload heap buffer, memcpy into it,
		// then CopyBufferRegion to the default heap buffer with the correct barriers.
		ComPtr<ID3D12Device> d3dDevice = dx12RHI->GetDevice();

		ComPtr<ID3D12Resource> uploadBuffer;
		const D3D12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		HRESULT hr = d3dDevice->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer));
		if (hr != S_OK) [[unlikely]]
		{
			TAssertf(false, "D3D12RHIIndexBuffer::Update: Failed to create upload buffer (HRESULT: 0x%08X).", static_cast<uint32>(hr));
			return;
		}

		// Map upload buffer and copy CPU data into it.
		void* mappedData = nullptr;
		const D3D12_RANGE readRange = { 0, 0 };
		hr = uploadBuffer->Map(0, &readRange, &mappedData);
		if (hr != S_OK) [[unlikely]]
		{
			TAssertf(false, "D3D12RHIIndexBuffer::Update: Failed to map upload buffer (HRESULT: 0x%08X).", static_cast<uint32>(hr));
			return;
		}
		memcpy(mappedData, Data, uploadBufferSize);
		uploadBuffer->Unmap(0, nullptr);

		auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCopyCommandContext_RHI());
		if (!dx12Context) [[unlikely]]
		{
			TAssertf(false, "D3D12RHIIndexBuffer::Update: Failed to get copy command context.");
			return;
		}

		auto* cmdList = dx12Context->GetCommandList().Get();

		// Barrier: COMMON -> COPY_DEST
		D3D12_RESOURCE_STATES oldState = static_cast<D3D12_RESOURCE_STATES>(GetState_RenderThread());
		if (oldState != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			CD3DX12_RESOURCE_BARRIER toCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
				IndexBuffer.Get(), oldState, D3D12_RESOURCE_STATE_COPY_DEST);
			cmdList->ResourceBarrier(1, &toCopyDest);
		}

		cmdList->CopyBufferRegion(IndexBuffer.Get(), 0, uploadBuffer.Get(), 0, uploadBufferSize);

		// Barrier: COPY_DEST -> INDEX_BUFFER
		CD3DX12_RESOURCE_BARRIER toIBState = CD3DX12_RESOURCE_BARRIER::Transition(
			IndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		cmdList->ResourceBarrier(1, &toIBState);
		SetState_RenderThread(static_cast<ERHIResourceState>(D3D12_RESOURCE_STATE_INDEX_BUFFER));

		dx12RHI->AddReleaseObject(uploadBuffer);
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
	        // dynamic texture use map unmap
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

	    // 3. Prepare subresource data for UpdateSubresources
	    std::vector<D3D12_SUBRESOURCE_DATA> subresourceData(subresourceCount);
	    UINT64 srcOffset = 0;

	    for (UINT arraySlice = 0; arraySlice < textureDesc.DepthOrArraySize; ++arraySlice)
	    {
	        for (UINT mip = 0; mip < textureDesc.MipLevels; ++mip)
	        {
	            UINT subresourceIndex = D3D12CalcSubresource(mip, arraySlice, 0, textureDesc.MipLevels, textureDesc.DepthOrArraySize);

	            // Calculate dimensions for this mip level
	            UINT mipWidth = std::max(uint64(1), textureDesc.Width >> mip);
	            UINT mipHeight = std::max(1u, textureDesc.Height >> mip);

	            uint32 bytesPerPixel = BytesPerPixelFromDXGIFormat(textureDesc.Format);
	            uint32 rowBytes = mipWidth * bytesPerPixel;
	            uint32 sliceBytes = rowBytes * mipHeight;

	            // Fill D3D12_SUBRESOURCE_DATA
	            subresourceData[subresourceIndex].pData = static_cast<const uint8*>(Data->Data) + srcOffset;
	            subresourceData[subresourceIndex].RowPitch = rowBytes;
	            subresourceData[subresourceIndex].SlicePitch = sliceBytes;

	            srcOffset += sliceBytes * textureDesc.DepthOrArraySize;
	        }
	    }

	    // 4. Use UpdateSubresources to copy data from CPU to GPU
	    if (auto dx12Context = static_cast<D3D12CommandContext*>(IRHIModule::GetModule()->GetCopyCommandContext_RHI()))
	    {
			// sync update resource update in render thread, async update resource has double buffer so has no issues.
	    	D3D12_RESOURCE_STATES oldState = static_cast<D3D12_RESOURCE_STATES>(GetState_RenderThread());
	    	if (oldState != D3D12_RESOURCE_STATE_COPY_DEST)
	    	{
	    		CD3DX12_RESOURCE_BARRIER barrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(Texture.Get(), oldState, D3D12_RESOURCE_STATE_COPY_DEST);
	    		dx12Context->GetCommandList()->ResourceBarrier(1, &barrierDesc);
	    		SetState_RenderThread(static_cast<ERHIResourceState>(D3D12_RESOURCE_STATE_COPY_DEST));
	    	}
	        UpdateSubresources(dx12Context->GetCommandList().Get(), Texture.Get(), uploadBuffer.Get(), 0, 0, subresourceCount, subresourceData.data());
	    }
	    else
	    {
		    TAssertf(false, "fail to get dx12 command context");
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
