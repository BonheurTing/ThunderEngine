#include "RenderResource.h"

#include "IDynamicRHI.h"
#include "Concurrent/TaskScheduler.h"

namespace Thunder
{
	static TArray<RHIResource*>GRHIUpdateSyncQueue {};
	static TArray<RHIResource*> GRHIUpdateAsyncQueue {};
	
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
		
		TextureRHI = RHICreateTexture(desc, CreationFlags);

		if (!TextureRHI.IsValid() || MipLevels > 1)
		{
			return;
		}

		/**
		 * 把binary data放在 rhi resource中，防止rhi线程访问渲染线程的数据
		 **/
		GRHIScheduler->PushTask([renderRes = this, bin = RawData, rhiRes = TextureRHI]()
		{
			rhiRes->SetBinaryData(bin);
			GRHIUpdateSyncQueue.push_back(rhiRes);
		});
	}
}
