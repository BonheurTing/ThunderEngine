#include "RenderTargetPool.h"

namespace Thunder
{
    FGRenderTarget::FGRenderTarget(NameHandle name, uint32 width, uint32 height, RHIFormat format)
        : Name(name), Desc(width, height, format)
    {
    }

    FGRenderTarget::FGRenderTarget(NameHandle name, uint32 width, uint32 height, RHIFormat format, const TVector4f& clearValue)
        : Name(name), Desc(width, height, format, clearValue)
    {
    }

    RenderTextureRef RenderTargetPool::AcquireRenderTarget(FGRenderTargetDesc& desc)
    {
        // Find compatible render target in available pool
        for (auto it = AvailableTargets.begin(); it != AvailableTargets.end(); ++it)
        {
            RenderTexture* rt = it->RenderTarget.Get();

            if (rt->GetSizeX() != desc.Width || rt->GetSizeY() != desc.Height)
            {
                continue;
            }

            const RHITextureRef& textureRHI = rt->GetTextureRHI();
            if (!textureRHI.IsValid() || textureRHI->GetResourceDescriptor()->Format != desc.Format)
            {
                continue;
            }

            if (desc.bIsDepthStencil != rt->IsDepthStencilTargetable())
            {
                continue;
            }

            RenderTextureRef result = it->RenderTarget;
            AvailableTargets.erase(it);
            UsedTargets.push_back(result);
            return result;
        }

        // Create new render target if no compatible one found.
        ETextureCreateFlags flags = ETextureCreateFlags::Static;
        if (desc.bIsDepthStencil)
        {
            flags = static_cast<ETextureCreateFlags>(
                static_cast<uint32>(flags) | static_cast<uint32>(ETextureCreateFlags::DepthStencilTargetable));
        }
        else
        {
            flags = static_cast<ETextureCreateFlags>(
                static_cast<uint32>(flags) | static_cast<uint32>(ETextureCreateFlags::RenderTargetable));
        }

        RenderTexture2D* newTarget2D = new RenderTexture2D(desc.Width, desc.Height, desc.Format, nullptr, flags);
        if (desc.bHasClearValue)
        {
            TOptimizedClearValue clearValue;
            clearValue.bIsValid  = true;
            clearValue.Color[0]  = desc.ClearValue.X;
            clearValue.Color[1]  = desc.ClearValue.Y;
            clearValue.Color[2]  = desc.ClearValue.Z;
            clearValue.Color[3]  = desc.ClearValue.W;
            clearValue.Depth     = desc.ClearDepth;
            clearValue.Stencil   = desc.ClearStencil;
            newTarget2D->SetOptimizedClearValue(clearValue);
            desc.bHasClearValue = false;
        }
        RenderTextureRef newTarget = newTarget2D;
        newTarget->InitRHI();

        UsedTargets.push_back(newTarget);
        return newTarget;
    }

    void RenderTargetPool::ReleaseRenderTarget(const RenderTextureRef& renderTarget)
    {
        auto it = std::ranges::find(UsedTargets, renderTarget);
        if (it != UsedTargets.end())
        {
            AvailableTargets.emplace_back(*it);
            UsedTargets.erase(it);
        }
    }

    void RenderTargetPool::TickUnusedFrameCounters()
    {
        for (auto& pooledTarget : AvailableTargets)
        {
            pooledTarget.UnusedFrameCount++;
        }
    }

    void RenderTargetPool::ReleaseLongUnusedTargets()
    {
        uint32 releasedCount = 0;
        auto it = AvailableTargets.begin();

        while (it != AvailableTargets.end() && releasedCount < RENDERTARGET_MAX_RELEASE_PER_FRAME)
        {
            if (it->UnusedFrameCount >= RENDERTARGET_UNUSED_FRAME_THRESHOLD)
            {
                it = AvailableTargets.erase(it);
                releasedCount++;
            }
            else
            {
                ++it;
            }
        }
    }

    void RenderTargetPool::Reset()
    {
        AvailableTargets.clear();
        UsedTargets.clear();
    }
    
}
