#include "RenderTargetPool.h"

namespace Thunder
{
    uint32 FGRenderTarget::NextID = 1;

    FGRenderTarget::FGRenderTarget(uint32 Width, uint32 Height, EPixelFormat Format)
        : Desc(Width, Height, Format), ID(NextID++)
    {
    }

    RenderTextureRef RenderTargetPool::AcquireRenderTarget(const FGRenderTargetDesc& Desc)
    {
        // Find compatible render target in available pool
        for (auto it = AvailableTargets.begin(); it != AvailableTargets.end(); ++it)
        {
            auto& pooledTarget = *it;
            if (pooledTarget.RenderTarget->GetSizeX() == Desc.Width &&
                pooledTarget.RenderTarget->GetSizeY() == Desc.Height)
            {
                RenderTextureRef result = pooledTarget.RenderTarget;
                AvailableTargets.erase(it);
                UsedTargets.push_back(result);
                return result;
            }
        }

        // Create new render target if no compatible one found
        RHIFormat rhiFormat = RHIFormat::UNKNOWN;
        switch (Desc.Format)
        {
        case EPixelFormat::RGBA8888:
            rhiFormat = RHIFormat::R8G8B8A8_UNORM;
            break;
        case EPixelFormat::RGBA16F:
            rhiFormat = RHIFormat::R16G16B16A16_FLOAT;
            break;
        case EPixelFormat::RGBA32F:
            rhiFormat = RHIFormat::R32G32B32A32_FLOAT;
            break;
        default:
            break;
        }

        RenderTextureRef newTarget = new RenderTexture2D(
            Desc.Width, Desc.Height, rhiFormat, nullptr);
        UsedTargets.push_back(newTarget);
        return newTarget;
    }

    void RenderTargetPool::ReleaseRenderTarget(RenderTextureRef RenderTarget)
    {
        auto it = std::find(UsedTargets.begin(), UsedTargets.end(), RenderTarget);
        if (it != UsedTargets.end())
        {
            AvailableTargets.emplace_back(PooledRenderTarget(*it));
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
