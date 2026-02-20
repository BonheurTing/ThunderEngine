#pragma once

#include "RenderTexture.h"
#include "Vector.h"

namespace Thunder
{
    struct FGRenderTargetDesc
    {
        uint32 Width = 0;
        uint32 Height = 0;
        RHIFormat Format = RHIFormat::UNKNOWN;
        bool bIsDepthStencil = false;

        bool bHasClearValue = false;
        TVector4f ClearValue = TVector4f(0.f, 0.f, 0.f, 1.f);
        float ClearDepth = 1.f;
        uint8 ClearStencil = 0;

        FGRenderTargetDesc() = default;
        FGRenderTargetDesc(uint32 inWidth, uint32 inHeight, RHIFormat inFormat, bool bInIsDepthStencil = false)
            : Width(inWidth), Height(inHeight), Format(inFormat), bIsDepthStencil(bInIsDepthStencil)
        {
            bIsDepthStencil = IsDepthStencilFormat(inFormat);
        }
        FGRenderTargetDesc(uint32 inWidth, uint32 inHeight, RHIFormat inFormat, const TVector4f& inClearValue)
            : Width(inWidth), Height(inHeight), Format(inFormat), bHasClearValue(true), ClearValue(inClearValue)
        {
            bIsDepthStencil = IsDepthStencilFormat(inFormat);
        }
        FGRenderTargetDesc(uint32 inWidth, uint32 inHeight, RHIFormat inFormat, float inClearDepth, uint8 inClearStencil)
            : Width(inWidth), Height(inHeight), Format(inFormat), bHasClearValue(true), ClearDepth(inClearDepth), ClearStencil(inClearStencil)
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
        FGRenderTarget(uint32 width, uint32 height, RHIFormat format, const TVector4f& clearValue);

        FGRenderTargetDesc& GetDesc() { return Desc; }
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
        RenderTextureRef AcquireRenderTarget(FGRenderTargetDesc& desc);
        void ReleaseRenderTarget(const RenderTextureRef& renderTarget);
        void TickUnusedFrameCounters();
        void ReleaseLongUnusedTargets();
        void Reset();

    private:
        TArray<PooledRenderTarget> AvailableTargets;
        TArray<RenderTextureRef> UsedTargets;
    };
}
