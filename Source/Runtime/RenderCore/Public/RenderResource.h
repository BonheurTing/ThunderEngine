#pragma once
#include "RHIResource.h"

namespace Thunder
{
	class RENDERCORE_API RenderResource : public RefCountedObject
	{
	public:
		RenderResource() = default;
		virtual ~RenderResource() = default;

		virtual void InitRHI() {}
		virtual void ReleaseRHI() {}

		virtual void InitResource();
		virtual void ReleaseResource();
		void UpdateRHI();

		/*virtual bool IsValid() const = 0;
		virtual size_t GetMemorySize() const = 0;*/
	};

	class RenderTexture : public RenderResource
	{
	public:
		RHITextureRef TextureRHI;

		const RHITextureRef& GetTextureRHI() { return TextureRHI; }
		virtual uint32 GetSizeX() const { return 0; }
		virtual uint32 GetSizeY() const { return 0; }
		virtual uint32 GetSizeZ() const { return 0; }

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
		//todo ETextureCreateFlags  CreationFlags = TexCreate_None;
		bool bDynamic = false;
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
		void CreateTexture_RenderThread() final override;
	};
	
}

