#pragma once
#include "RHIResource.h"

namespace Thunder
{
	class RENDERCORE_API RenderResource : public RefCountedObject
	{
	public:
		RenderResource() = default;
		virtual ~RenderResource() = default;

		// render thread
		virtual void InitResource();
		virtual void ReleaseResource();

		virtual void InitRHI() {}
		virtual void ReleaseRHI() {}
		void UpdateRHI();

		/*virtual bool IsValid() const = 0;
		virtual size_t GetMemorySize() const = 0;*/
		virtual bool isDynamic() const { return false; }
		virtual bool isDoubleBuffered() const { return false; }
	};

	extern RENDERCORE_API TArray<RHIResource*> GRHIUpdateSyncQueue;
	extern RENDERCORE_API TArray<RHIResource*> GRHIUpdateAsyncQueue;
}

