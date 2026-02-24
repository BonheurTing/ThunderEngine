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

        FORCEINLINE void SetResolution(TVector2u newResolution) { Width = newResolution.X; Height = newResolution.Y; }
    };

    class FGRenderTarget : public RefCountedObject
    {
    public:
        RENDERCORE_API FGRenderTarget() = default;
        RENDERCORE_API FGRenderTarget(NameHandle name, uint32 width, uint32 height, RHIFormat format);
        RENDERCORE_API FGRenderTarget(NameHandle name, uint32 width, uint32 height, RHIFormat format, const TVector4f& clearValue);
        RENDERCORE_API FGRenderTarget(NameHandle name, uint32 width, uint32 height, RHIFormat format, float clearDepth, uint8 clearStencil);

        FORCEINLINE NameHandle GetName() const { return Name; }
        FORCEINLINE FGRenderTargetDesc& GetDesc() { return Desc; }
        FORCEINLINE uint32 GetWidth() const { return Desc.Width; }
        FORCEINLINE uint32 GetHeight() const { return Desc.Height; }
        FORCEINLINE RHIFormat GetFormat() const { return Desc.Format; }

        FORCEINLINE uint32 GetID() const { return CurrentFrameTargetId; }
        FORCEINLINE void SetId(uint32 inID) { CurrentFrameTargetId = inID; }
        FORCEINLINE bool IsSame(const FGRenderTarget& other) const { return Desc == other.Desc; }
        FORCEINLINE bool IsSame(const FGRenderTarget* other) const { return Desc == other->Desc; }
        FORCEINLINE void SetResolution(TVector2u newResolution) { Desc.SetResolution(newResolution); }

    private:
        NameHandle Name;
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
