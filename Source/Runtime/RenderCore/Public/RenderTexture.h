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

    class RenderTexture2D : public RenderTexture
    {
    public:
        RenderTexture2D(uint32 width, uint32 height, RHIFormat format, const BinaryDataRef& inData)
        {
            SizeX = width;
            SizeY = height;
            PixelFormat = format;
            RawData = inData;
        }
    private:
        void CreateTexture_RenderThread() final;
    };
}
