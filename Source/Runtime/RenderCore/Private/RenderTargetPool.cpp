#include "RenderTargetPool.h"

namespace Thunder
{
    uint32 FGRenderTarget::NextID = 1;

    FGRenderTarget::FGRenderTarget(uint32 width, uint32 height, EPixelFormat format)
        : Desc(width, height, format), ID(NextID++)
    {
    }

    RenderTextureRef RenderTargetPool::AcquireRenderTarget(const FGRenderTargetDesc& desc)
    {
        // Find compatible render target in available pool
        for (auto it = AvailableTargets.begin(); it != AvailableTargets.end(); ++it)
        {
            auto& pooledTarget = *it;
            if (pooledTarget.RenderTarget->GetSizeX() == desc.Width &&
                pooledTarget.RenderTarget->GetSizeY() == desc.Height)
            {
                RenderTextureRef result = pooledTarget.RenderTarget;
                AvailableTargets.erase(it);
                UsedTargets.push_back(result);
                return result;
            }
        }

        // Create new render target if no compatible one found
        RHIFormat rhiFormat = RHIFormat::UNKNOWN;
        switch (desc.Format)
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
        case EPixelFormat::D32S8X24:
            rhiFormat = RHIFormat::D32_FLOAT_S8X24_UINT;
            break;
        default:
            break;
        }

        RenderTextureRef newTarget = new RenderTexture2D(
            desc.Width, desc.Height, rhiFormat, nullptr);
        UsedTargets.push_back(newTarget);
        return newTarget;
    }

    void RenderTargetPool::ReleaseRenderTarget(RenderTextureRef renderTarget)
    {
        auto it = std::find(UsedTargets.begin(), UsedTargets.end(), renderTarget);
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
