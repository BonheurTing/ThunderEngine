#pragma once
#include "RHIResource.h"

namespace Thunder
{
	class RenderResource : public RefCountedObject
	{
	public:
		RENDERCORE_API RenderResource() = default;
		RENDERCORE_API virtual ~RenderResource() = default;

		// render thread
		RENDERCORE_API virtual void InitResource();
		RENDERCORE_API virtual void ReleaseResource();

		RENDERCORE_API virtual void InitRHI() {}
		RENDERCORE_API virtual void ReleaseRHI() {}
		RENDERCORE_API void UpdateRHI();

		/*virtual bool IsValid() const = 0;
		virtual size_t GetMemorySize() const = 0;*/
		RENDERCORE_API virtual bool isDynamic() const { return false; }
		RENDERCORE_API virtual bool isDoubleBuffered() const { return false; }
	};

	extern RENDERCORE_API TArray<RHIResource*> GRHIUpdateSyncQueue;
	extern RENDERCORE_API TArray<RHIResource*> GRHIUpdateAsyncQueue;
}

