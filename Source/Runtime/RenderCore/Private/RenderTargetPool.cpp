#include "RenderTargetPool.h"

namespace Thunder
{
    FGRenderTarget::FGRenderTarget(uint32 width, uint32 height, RHIFormat format)
        : Desc(width, height, format)
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

        // Create new render target if no compatible one found.
        RenderTextureRef newTarget = new RenderTexture2D(
            desc.Width, desc.Height, desc.Format, nullptr);
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
