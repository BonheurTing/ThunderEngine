#pragma optimize("", off)
#include "D3D12CommandContext.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12PipelineState.h"
#include "D3D12Resource.h"

namespace Thunder
{
	void D3D12CommandContext::ClearDepthStencilView(RHIDepthStencilView* dsv, ERHIClearFlags clearFlags, float depthValue, uint8 stencilValue)
	{
		if (const auto d3d12Dsv = static_cast<D3D12RHIDepthStencilView*>(dsv))
		{
			CommandList->ClearDepthStencilView(d3d12Dsv->GetCPUHandle(), static_cast<D3D12_CLEAR_FLAGS>(clearFlags), depthValue, stencilValue, 0, nullptr);
		}
	}

	void D3D12CommandContext::ClearRenderTargetView(RHIRenderTargetView* rtv, TVector4f clearColor)
	{
		if (const auto d3d12Rtv = static_cast<D3D12RHIRenderTargetView*>(rtv))
		{
			CommandList->ClearRenderTargetView(d3d12Rtv->GetCPUHandle(), clearColor.XYZW, 0, nullptr);
		}
	}

	void D3D12CommandContext::ClearState(TRHIPipelineState* pso)
	{
		if (pso->GetPipelineStateType() == ERHIPipelineStateType::Graphics)
		{
			if (const auto d2d12Pso = static_cast<TD3D12GraphicsPipelineState*>(pso))
			{
				CommandList->ClearState(static_cast<ID3D12PipelineState*>(d2d12Pso->GetPipelineState()));
			}
		}
		else if (pso->GetPipelineStateType() == ERHIPipelineStateType::Compute)
		{
			if (const auto d2d12Pso = static_cast<TD3D12ComputePipelineState*>(pso))
			{
				CommandList->ClearState(static_cast<ID3D12PipelineState*>(d2d12Pso->GetPipelineState()));
			}
		}
		else
		{
			TAssertf(false, "Unknown pipeline state type");
		}
	}

	void D3D12CommandContext::ClearUnorderedAccessViewUint(RHIResource* resource, TVector4u clearValue)
	{
		if (const auto d3d12Resource = static_cast<ID3D12Resource*>(resource->GetResource()))
		{
			if (const auto d3d12Uav = static_cast<D3D12RHIUnorderedAccessView*>(resource->GetUAV().Get()))
			{
				CommandList->ClearUnorderedAccessViewUint(d3d12Uav->GetGPUHandle(), d3d12Uav->GetCPUHandle(), d3d12Resource, clearValue.XYZW, 0, nullptr);
			}
		}
	}

	void D3D12CommandContext::ClearUnorderedAccessViewFloat(RHIResource* resource, TVector4f clearValue)
	{
		if (const auto d3d12Resource = static_cast<ID3D12Resource*>(resource->GetResource()))
		{
			if (const auto d3d12Uav = static_cast<D3D12RHIUnorderedAccessView*>(resource->GetUAV().Get()))
			{
				CommandList->ClearUnorderedAccessViewFloat(d3d12Uav->GetGPUHandle(), d3d12Uav->GetCPUHandle(), d3d12Resource, clearValue.XYZW, 0, nullptr);
			}
		}
	}

	void D3D12CommandContext::SetIndexBuffer(RHIIndexBufferRef indexBuffer)
	{
		if (const auto d3d12IndexBuffer = static_cast<D3D12RHIIndexBuffer*>(indexBuffer.Get()))
		{
			CommandList->IASetIndexBuffer(d3d12IndexBuffer->GetIndexBufferView());
		}
	}

	void D3D12CommandContext::SetPrimitiveTopology(ERHIPrimitive type)
	{
		CommandList->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(type));
	}

	void D3D12CommandContext::SetVertexBuffer(uint32 slot, uint32 numViews, RHIVertexBufferRef vertexBuffer)
	{
		if (const auto d3d12VertexBuffer = static_cast<D3D12RHIVertexBuffer*>(vertexBuffer.Get()))
		{
			CommandList->IASetVertexBuffers(slot, numViews, d3d12VertexBuffer->GetVertexBufferView());
		}
	}

	void D3D12CommandContext::SetBlendFactor(TVector4f const& blendFactor)
	{
		CommandList->OMSetBlendFactor(blendFactor.XYZW);
	}

	void D3D12CommandContext::SetRenderTarget(uint32 numRT, TArray<RHIRenderTargetView*> rtvs, RHIDepthStencilView* dsv)
	{
		TAssertf(numRT <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "Too many render targets");
		TArray<D3D12_CPU_DESCRIPTOR_HANDLE> d3d12Rtvs;
		for (const auto rtv : rtvs)
		{
			if (const auto d3d12Rtv = static_cast<D3D12RHIRenderTargetView*>(rtv))
			{
				d3d12Rtvs.push_back(d3d12Rtv->GetCPUHandle());
			}
		}
		if (const auto d3d12Dsv = static_cast<D3D12RHIDepthStencilView*>(dsv))
		{
			const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = d3d12Dsv->GetCPUHandle();
			CommandList->OMSetRenderTargets(numRT, d3d12Rtvs.data(), false, &dsvHandle);
		}
		else
		{
			CommandList->OMSetRenderTargets(numRT, d3d12Rtvs.data(), false, nullptr);
		}
	}

	void D3D12CommandContext::SetScissorRects(TArray<RHIRect*> rects)
	{
		TArray<D3D12_RECT> d3d12Rects;
		for (const auto rect : rects)
		{
			const auto vp = reinterpret_cast<D3D12_RECT*>(rect);
			d3d12Rects.push_back(*vp);
		}
		CommandList->RSSetScissorRects(static_cast<UINT>(rects.size()), d3d12Rects.data());
	}

	void D3D12CommandContext::SetViewports(TArray<RHIViewport*> viewports)
	{
		TArray<D3D12_VIEWPORT> d3d12Viewports;
		for (const auto viewport : viewports)
		{
			const auto vp = static_cast<const D3D12_VIEWPORT*>(viewport->GetViewPort());
			d3d12Viewports.push_back(*vp);
		}
		CommandList->RSSetViewports(static_cast<UINT>(viewports.size()), d3d12Viewports.data());
	}

	void D3D12CommandContext::SetPipelineState(TRHIPipelineState* pso)
	{
		CommandList->SetPipelineState(static_cast<ID3D12PipelineState*>(pso->GetPipelineState()));
	}

	void D3D12CommandContext::CopyBufferRegion(RHIResource* dst, uint64 dstOffset, RHIResource* src, uint64 srcOffset, uint64 numBytes)
	{
		const auto d3d12Dst = static_cast<ID3D12Resource*>(dst->GetResource());
		const auto d3d12Src = static_cast<ID3D12Resource*>(src->GetResource());
		if (dst!=nullptr && src!=nullptr)
		{
			const RHIResourceDescriptor* dstDesc = dst->GetResourceDescriptor();
			const RHIResourceDescriptor* srcDesc = src->GetResourceDescriptor();
			TAssertf(dstDesc->Format == srcDesc->Format, "Resource format mismatch");
			TAssertf(dstDesc->Type == srcDesc->Type, "Resource type mismatch");
			TAssertf(dstDesc->Type == ERHIResourceType::Buffer, "Resource is not buffer");
			CommandList->CopyBufferRegion(d3d12Dst, dstOffset, d3d12Src, srcOffset, numBytes);
		}
	}

	void D3D12CommandContext::CopyTextureRegion(RHIResource* dst, uint32 dstMip, RHIResource* src, uint32 srcMip, const RHITextureRegion* copyRegion)
	{
		const auto d3d12Dst = static_cast<ID3D12Resource*>(dst->GetResource());
		const auto d3d12Src = static_cast<ID3D12Resource*>(src->GetResource());
		if (dst!=nullptr && src!=nullptr)
		{
			TAssertf(dst != src || dstMip != srcMip, "Source and destination sub resources are the same");
			const RHIResourceDescriptor* dstDesc = dst->GetResourceDescriptor();
			const RHIResourceDescriptor* srcDesc = src->GetResourceDescriptor();
			
			TAssertf(dstDesc->Format == srcDesc->Format, "Resource format mismatch");
			if (copyRegion != nullptr)
			{
				TAssertf(static_cast<uint32>(dstDesc->Width) / pow(2, dstMip) >= (copyRegion->DstX + copyRegion->Width), "sub resource size mismatch");
				TAssertf(static_cast<uint32>(dstDesc->Height) / pow(2, dstMip) >= (copyRegion->DstY + copyRegion->Height), "sub resource size mismatch");
				TAssertf(static_cast<uint32>(dstDesc->DepthOrArraySize) / pow(2, dstMip) >= (copyRegion->DstZ + copyRegion->DepthOrArraySize), "sub resource size mismatch");
				TAssertf(static_cast<uint32>(srcDesc->Width) / pow(2, srcMip) >= (copyRegion->SrcX + copyRegion->Width), "sub resource size mismatch");
				TAssertf(static_cast<uint32>(srcDesc->Height) / pow(2, srcMip) >= (copyRegion->SrcY + copyRegion->Height), "sub resource size mismatch");
				TAssertf(static_cast<uint32>(srcDesc->DepthOrArraySize) / pow(2, srcMip) >= (copyRegion->SrcZ + copyRegion->DepthOrArraySize), "sub resource size mismatch");
			}
			TAssertf(dstDesc->Type == srcDesc->Type, "Resource type mismatch");
			TAssertf(dstDesc->Type != ERHIResourceType::Buffer, "Resource is not buffer");
			const auto dstTexture = static_cast<const RHITexture*>(dst);
			const auto srcTexture = static_cast<const RHITexture*>(src);
			if(dstTexture->GetDSV() || srcTexture->GetDSV())
			{
				TAssertf(dstDesc->SampleDesc.Count == srcDesc->SampleDesc.Count, "Resource sample count mismatch");
				TAssertf(dstDesc->SampleDesc.Quality == srcDesc->SampleDesc.Quality, "Resource sample quality mismatch");
			}
			
			const CD3DX12_TEXTURE_COPY_LOCATION dstLocation(d3d12Dst, dstMip);
			const CD3DX12_TEXTURE_COPY_LOCATION srcLocation(d3d12Src, srcMip);
			D3D12_BOX sourceRegion;
			if (copyRegion != nullptr)
			{
				sourceRegion.left = copyRegion->SrcX;
				sourceRegion.top = copyRegion->SrcY;
				sourceRegion.right = copyRegion->SrcX + copyRegion->Width;
				sourceRegion.bottom = copyRegion->SrcY + copyRegion->Height;
				sourceRegion.front = copyRegion->SrcZ;
				sourceRegion.back = copyRegion->SrcZ + copyRegion->DepthOrArraySize;
				CommandList->CopyTextureRegion(&dstLocation, copyRegion->DstX, copyRegion->DstY, copyRegion->DstZ, &srcLocation, &sourceRegion);
			}
			else
			{
				CommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
			}
		}
	}

	void D3D12CommandContext::CopyResource(RHIResource* dst, RHIResource* src)
	{
		const auto d3d12Dst = static_cast<ID3D12Resource*>(dst->GetResource());
		const auto d3d12Src = static_cast<ID3D12Resource*>(src->GetResource());
		if (dst!=nullptr && src!=nullptr)
		{
			TAssertf(dst != src, "Source and destination resources are the same");
			const RHIResourceDescriptor* dstDesc = dst->GetResourceDescriptor();
			const RHIResourceDescriptor* srcDesc = src->GetResourceDescriptor();
			TAssertf(dstDesc->Format == srcDesc->Format, "Resource format mismatch");
			TAssertf(dstDesc->Width * dstDesc->Height * dstDesc->DepthOrArraySize == srcDesc->Width * srcDesc->Height * srcDesc->DepthOrArraySize, "Resource size mismatch");
			TAssertf(dstDesc->Type == srcDesc->Type, "Resource type mismatch");
			if(dstDesc->Type != ERHIResourceType::Buffer)
			{
				const auto dstTexture = static_cast<const RHITexture*>(dst);
				const auto srcTexture = static_cast<const RHITexture*>(src);
				if(dstTexture->GetDSV() || srcTexture->GetDSV())
				{
					TAssertf(dstDesc->SampleDesc.Count == srcDesc->SampleDesc.Count, "Resource sample count mismatch");
					TAssertf(dstDesc->SampleDesc.Quality == srcDesc->SampleDesc.Quality, "Resource sample quality mismatch");
				}
			}
			
			CommandList->CopyResource(d3d12Dst, d3d12Src);
		}
	}

	void D3D12CommandContext::DiscardResource(RHIResource* resource, TArray<RHIRect> const& rects)
	{
		if (const auto d3d12Resource = static_cast<ID3D12Resource*>(resource->GetResource()))
		{
			TArray<D3D12_RECT> d3d12Rects;
			for (const auto rect : rects)
			{
				D3D12_RECT d3d12Rect;
				d3d12Rect.left = rect.Left;
				d3d12Rect.top = rect.Top;
				d3d12Rect.right = rect.Right;
				d3d12Rect.bottom = rect.Bottom;
				TAssertf(static_cast<UINT64>(rect.Left+rect.Right) <= resource->GetResourceDescriptor()->Width, "Rect is out of bounds");
				TAssertf(static_cast<UINT>(rect.Top+rect.Bottom) <= resource->GetResourceDescriptor()->Height, "Rect is out of bounds");
				d3d12Rects.push_back(d3d12Rect);
			}
			const D3D12_DISCARD_REGION region = {static_cast<UINT>(d3d12Rects.size()), d3d12Rects.data(), 0, resource->GetResourceDescriptor()->MipLevels};
			CommandList->DiscardResource(d3d12Resource, &region);
		}
	}

	void D3D12CommandContext::ResolveSubresource(RHIResource* dst, uint32 dstSubId, RHIResource* src, uint32 srcSubId)
	{
		TAssertf(dst!=nullptr && src!=nullptr, "Source or destination resource is null");
		const auto d3d12Dst = static_cast<ID3D12Resource*>(dst->GetResource());
		const auto d3d12Src = static_cast<ID3D12Resource*>(src->GetResource());
		TAssertf(d3d12Dst!= nullptr && d3d12Src != nullptr, "Source or destination d3d12 resource is null");
		const RHIResourceDescriptor* dstDesc = dst->GetResourceDescriptor();
		TAssertf(dstDesc->SampleDesc.Count == 1, "Destination resource is not single-sampled");
		const RHIResourceDescriptor* srcDesc = src->GetResourceDescriptor();
		TAssertf(srcDesc->SampleDesc.Count > 1, "Source resource is not multi-sampled");
		CommandList->ResolveSubresource(d3d12Dst, dstSubId, d3d12Src, srcSubId, static_cast<DXGI_FORMAT>(srcDesc->Format));
	}

	void D3D12CommandContext::Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
	{
		CommandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void D3D12CommandContext::DrawIndexedInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation, int32 baseVertexLocation, uint32 startInstanceLocation)
	{
		CommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}

	void D3D12CommandContext::DrawInstanced(uint32 vertexCountPerInstance, uint32 instanceCount, uint32 startVertexLocation, uint32 startInstanceLocation)
	{
		CommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
	}

	void D3D12CommandContext::Execute()
	{
		ID3D12CommandList* ppCommandLists[] = { CommandList.Get() };
		CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	void D3D12CommandContext::Close()
	{
		const HRESULT hr = CommandList->Close();
		TAssertf(SUCCEEDED(hr), "Failed to close command list");
	}

	void D3D12CommandContext::Reset()
	{
		HRESULT hr = CommandAllocator->Reset();
		TAssertf(SUCCEEDED(hr), "Failed to reset command allocator");
		hr = CommandList->Reset(CommandAllocator.Get(), nullptr);
		TAssertf(SUCCEEDED(hr), "Failed to reset command list");
	}
}
