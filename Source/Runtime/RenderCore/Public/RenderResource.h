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

		void ReleaseRHI() override
		{
			TextureRHI.SafeRelease();
		}
	};
	using RenderTextureRef = TRefCountPtr<RenderTexture>;
}

