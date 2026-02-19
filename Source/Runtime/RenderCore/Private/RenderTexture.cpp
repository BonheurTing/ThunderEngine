#include "RenderTexture.h"
#include "IDynamicRHI.h"
#include "Concurrent/TaskScheduler.h"

namespace Thunder
{
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

        const bool bNeedRTV = IsRenderTargetable();
        const bool bNeedDSV = IsDepthStencilTargetable();

        RHIResourceDescriptor desc{};
        desc.Type = ERHIResourceType::Texture2D;
        desc.Alignment = 0;
        desc.Width = SizeX;
        desc.Height = SizeY;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = MipLevels;
        desc.Format = PixelFormat;
        desc.SampleDesc = {1, 0};
        desc.Layout = ERHITextureLayout::RowMajor;
        desc.Flags = {};
        desc.Flags.NeedRTV = bNeedRTV;
        desc.Flags.NeedDSV = bNeedDSV;

        TextureRHI = RHICreateTexture(desc, CreationFlags);

        RHICreateShaderResourceView(*TextureRHI, {
            .Format = desc.Format,
            .Type = ERHIViewDimension::Texture2D,
            .Width = desc.Width,
            .Height = desc.Height,
            .DepthOrArraySize = desc.DepthOrArraySize
        });

        if (bNeedRTV)
        {
            RHICreateRenderTargetView(*TextureRHI, {
                .Format = desc.Format,
                .Type = ERHIViewDimension::Texture2D,
                .Width = desc.Width,
                .Height = desc.Height,
                .DepthOrArraySize = desc.DepthOrArraySize
            });
        }

        if (bNeedDSV)
        {
            RHICreateDepthStencilView(*TextureRHI, {
                .Format = desc.Format,
                .Type = ERHIViewDimension::Texture2D,
                .Width = desc.Width,
                .Height = desc.Height,
                .DepthOrArraySize = desc.DepthOrArraySize
            });
        }

        if (!TextureRHI.IsValid() || MipLevels > 1)
        {
            return;
        }

        // Store the binary data inside the RHI resource
        // to prevent the RHI thread from accessing data owned by the render thread.
        if (RawData.IsValid())
        {
            GRHIScheduler->PushTask([bin = RawData, rhiRes = TextureRHI, bAsync = isDoubleBuffered() || (!isDynamic())]()
            {
                rhiRes->SetBinaryData(bin);
                if (bAsync)
                {
                    GRHIUpdateAsyncQueue.push_back(rhiRes);
                }
                else
                {
                    GRHIUpdateSyncQueue.push_back(rhiRes);
                }
            });
        }
    }
}
