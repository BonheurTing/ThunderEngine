#pragma once

#include "RenderTexture.h"

namespace Thunder
{
    struct FGRenderTargetDesc
    {
        uint32 Width = 0;
        uint32 Height = 0;
        RHIFormat Format = RHIFormat::UNKNOWN;
        bool bIsDepthStencil = false;

        FGRenderTargetDesc() = default;
        FGRenderTargetDesc(uint32 inWidth, uint32 inHeight, RHIFormat inFormat, bool bInIsDepthStencil = false)
            : Width(inWidth), Height(inHeight), Format(inFormat), bIsDepthStencil(bInIsDepthStencil)
        {
            bIsDepthStencil = IsDepthStencilFormat(inFormat);
        }

        bool operator==(const FGRenderTargetDesc& other) const
        {
            return Width == other.Width && Height == other.Height &&
                   Format == other.Format && bIsDepthStencil == other.bIsDepthStencil;
        }
    };

    class RENDERCORE_API FGRenderTarget : public RefCountedObject
    {
    public:
        FGRenderTarget() = default;
        FGRenderTarget(uint32 width, uint32 height, RHIFormat format);

        const FGRenderTargetDesc& GetDesc() const { return Desc; }
        uint32 GetWidth() const { return Desc.Width; }
        uint32 GetHeight() const { return Desc.Height; }
        RHIFormat GetFormat() const { return Desc.Format; }

        uint32 GetID() const { return CurrentFrameTargetId; }
        void SetId(uint32 inID) { CurrentFrameTargetId = inID; }
        bool IsSame(const FGRenderTarget& other) const { return Desc == other.Desc; }
        bool IsSame(const FGRenderTarget* other) const { return Desc == other->Desc; }

    private:
        FGRenderTargetDesc Desc;
        uint32 CurrentFrameTargetId = 0xFFFFFFFF;
    };
    using FGRenderTargetRef = TRefCountPtr<FGRenderTarget>;

    #define RENDERTARGET_UNUSED_FRAME_THRESHOLD 60
    #define RENDERTARGET_MAX_RELEASE_PER_FRAME 5

    struct PooledRenderTarget
    {
        RenderTextureRef RenderTarget;
        uint32 UnusedFrameCount = 0;

        PooledRenderTarget(RenderTextureRef inRenderTarget)
            : RenderTarget(inRenderTarget) {}
    };

    class RENDERCORE_API RenderTargetPool
    {
    public:
        RenderTextureRef AcquireRenderTarget(const FGRenderTargetDesc& desc);
        void ReleaseRenderTarget(RenderTextureRef renderTarget);
        void TickUnusedFrameCounters();
        void ReleaseLongUnusedTargets();
        void Reset();

    private:
        TArray<PooledRenderTarget> AvailableTargets;
        TArray<RenderTextureRef> UsedTargets;
    };
}
