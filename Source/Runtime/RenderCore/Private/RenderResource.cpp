#include "RenderResource.h"

#include "IDynamicRHI.h"
#include "Concurrent/TaskScheduler.h"

namespace Thunder
{
	void RenderResource::InitResource()
	{
		//check(IsInRenderingThread());
		// if GIsRHIInitialized
		InitRHI();
	}

	void RenderResource::ReleaseResource()
	{
		//check(IsInRenderingThread());
		// if GIsRHIInitialized
		ReleaseRHI();
	}

	void RenderResource::UpdateRHI()
	{
		//check(IsInRenderingThread());
		ReleaseRHI();
		InitRHI();
	}

	void RenderTexture::InitRHI()
	{
		CreateTexture_RenderThread();
	}

	void RenderTexture::ReleaseRHI()
	{
		TextureRHI.SafeRelease();
	}

	void RenderTexture2D::CreateTexture_RenderThread()
	{
		/**
		 * 1.fill in the desc (tips：flag：dynamic or not)
		 * 2.TextureRHI = RHICreateTexture(Desc);
		 * 3.RHILockTexture2D
		 * 4.memcpy
		 * 5.RHILockTexture2D
		 * 6.CopyTetxureRegion
		 **/

		RHIResourceDescriptor desc {};
		desc.Type = ERHIResourceType::Texture2D;
		desc.Alignment = 0;
		desc.Width = SizeX;
		desc.Height = SizeY;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = MipLevels;
		desc.Format = PixelFormat;
		desc.SampleDesc = {1, 0};
		desc.Layout = ERHITextureLayout::RowMajor;
		desc.Flags = {0, 0, 0, 0, 0};
		
		TextureRHI = RHICreateTexture2D(desc, bDynamic ? EResourceUsageFlags::Dynamic : EResourceUsageFlags::None);
		RHITexture2D* dstTexture = static_cast<RHITexture2D*>(TextureRHI.Get());

		if (dstTexture == nullptr || MipLevels > 1)
		{
			return;
		}

		
		for (uint16 mipId = 0; mipId < MipLevels; ++mipId)
		{
			uint32 dstStride;
			void* mipData = RHIMapTexture2D(dstTexture, mipId, 0/*RLM_WriteOnly*/, dstStride);

			memcpy(mipData, RawData->Data, RawData->Size);

			RHIUnmapTexture2D(dstTexture, mipId );
			// RHIUpdateTexture(dstTexture);
			GRHIScheduler->PushTask([]()
			{
				GRHISyncQueue->push(RHIUpdateTexture(dstTexture));
			});
		}
	}
}
