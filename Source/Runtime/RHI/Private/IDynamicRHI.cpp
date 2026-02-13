#include "IDynamicRHI.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
	IDynamicRHI* GDynamicRHI = nullptr;
	std::atomic_uint64_t GCachedMeshDrawCommandIndex = 0;

	void IDynamicRHI::DeferredDeleteResource(TRefCountPtr<RHIResource> resource)
	{
		uint32 index = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % MAX_FRAME_LAG;
		DeferredDeleteQueue[index].push_back(std::move(resource));
	}

	void IDynamicRHI::FlushDeferredDeleteQueue()
	{
		uint32 index = (GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) + 1) % MAX_FRAME_LAG;
		DeferredDeleteQueue[index].clear();
	}
}
