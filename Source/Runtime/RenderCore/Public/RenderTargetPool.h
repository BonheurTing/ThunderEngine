#pragma once

#include "RenderTexture.h"

namespace Thunder
{
    enum class EPixelFormat : uint8
    {
        UNKNOWN,
        RGBA8888,
        RGBA16F,
        RGBA32F,
        RGB10A2,
        R16F,
        R32F,
        D24S8,
        D32F
    };

    struct FGRenderTargetDesc
    {
        uint32 Width = 0;
        uint32 Height = 0;
        EPixelFormat Format = EPixelFormat::UNKNOWN;
        bool bIsDepthStencil = false;

        FGRenderTargetDesc() = default;
        FGRenderTargetDesc(uint32 InWidth, uint32 InHeight, EPixelFormat InFormat, bool bInIsDepthStencil = false)
            : Width(InWidth), Height(InHeight), Format(InFormat), bIsDepthStencil(bInIsDepthStencil) {}

        bool operator==(const FGRenderTargetDesc& Other) const
        {
            return Width == Other.Width && Height == Other.Height &&
                   Format == Other.Format && bIsDepthStencil == Other.bIsDepthStencil;
        }
    };

    class RENDERCORE_API FGRenderTarget
    {
    public:
        FGRenderTarget() = default;
        FGRenderTarget(uint32 Width, uint32 Height, EPixelFormat Format);

        const FGRenderTargetDesc& GetDesc() const { return Desc; }
        uint32 GetWidth() const { return Desc.Width; }
        uint32 GetHeight() const { return Desc.Height; }
        EPixelFormat GetFormat() const { return Desc.Format; }

        uint32 GetID() const { return ID; }

    private:
        FGRenderTargetDesc Desc;
        uint32 ID = 0;
        static uint32 NextID;
    };

    #define RENDERTARGET_UNUSED_FRAME_THRESHOLD 60
    #define RENDERTARGET_MAX_RELEASE_PER_FRAME 5

    struct PooledRenderTarget
    {
        RenderTextureRef RenderTarget;
        uint32 UnusedFrameCount = 0;

        PooledRenderTarget(RenderTextureRef InRenderTarget)
            : RenderTarget(InRenderTarget) {}
    };

    class RENDERCORE_API RenderTargetPool
    {
    public:
        RenderTextureRef AcquireRenderTarget(const FGRenderTargetDesc& Desc);
        void ReleaseRenderTarget(RenderTextureRef RenderTarget);
        void TickUnusedFrameCounters();
        void ReleaseLongUnusedTargets();
        void Reset();

    private:
        TArray<PooledRenderTarget> AvailableTargets;
        TArray<RenderTextureRef> UsedTargets;
    };
}
