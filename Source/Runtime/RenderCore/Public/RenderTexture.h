#pragma once

#include "BinaryData.h"
#include "RenderResource.h"

namespace Thunder
{
    class RenderTexture : public RenderResource
    {
    public:
        RHITextureRef TextureRHI;

        const RHITextureRef& GetTextureRHI() { return TextureRHI; }
        virtual uint32 GetSizeX() const { return 0; }
        virtual uint32 GetSizeY() const { return 0; }
        virtual uint32 GetSizeZ() const { return 0; }
        bool isDynamic() const override { return (static_cast<uint32>(CreationFlags) & static_cast<uint32>(ETextureCreateFlags::AnyDynamic)) > 0; }
        bool IsRenderTargetable() const { return (static_cast<uint32>(CreationFlags) & static_cast<uint32>(ETextureCreateFlags::RenderTargetable)) > 0; }
        bool IsDepthStencilTargetable() const { return (static_cast<uint32>(CreationFlags) & static_cast<uint32>(ETextureCreateFlags::DepthStencilTargetable)) > 0; }

        void InitRHI() override;
        void ReleaseRHI() override;
    protected:
        virtual void CreateTexture_RenderThread() = 0;
        float MipBias = 0;

        uint32 SizeX = 0;
        uint32 SizeY = 0;
        uint32 SizeZ = 0;
        uint16 MipLevels = 1;

        NameHandle TextureName;
        RHIFormat PixelFormat = RHIFormat::UNKNOWN;
        ETextureCreateFlags CreationFlags = ETextureCreateFlags::Static;
        //bool bDynamic = false;
        BinaryDataRef RawData;
    };
    using RenderTextureRef = TRefCountPtr<RenderTexture>;

    struct TOptimizedClearValue
    {
        bool  bIsValid    = false;
        float Color[4]    = { 0.f, 0.f, 0.f, 1.f };
        float Depth       = 1.f;
        uint8 Stencil     = 0;
    };

    class RenderTexture2D : public RenderTexture
    {
    public:
        RenderTexture2D(uint32 width, uint32 height, RHIFormat format, const BinaryDataRef& inData,
                        ETextureCreateFlags flags = ETextureCreateFlags::Static)
        {
            SizeX = width;
            SizeY = height;
            PixelFormat = format;
            RawData = inData;
            CreationFlags = flags;
        }

        void SetOptimizedClearValue(const TOptimizedClearValue& clearValue) { OptimizedClear = clearValue; }

        uint32 GetSizeX() const override { return SizeX; }
        uint32 GetSizeY() const override { return SizeY; }
    private:
        void CreateTexture_RenderThread() final;
        TOptimizedClearValue OptimizedClear;
    };
}
