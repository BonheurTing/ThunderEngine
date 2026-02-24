#include "IDynamicRHI.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
	IDynamicRHI* GDynamicRHI = nullptr;
	std::atomic_uint64_t GCachedMeshDrawCommandIndex = 0;

	void IDynamicRHI::DeferredDeleteResource(TRefCountPtr<RHIResource> resource)
	{
		//render thread
		uint32 index = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % MAX_FRAME_LAG;
		DeferredDeleteQueue[index].push_back(std::move(resource));
	}

	void IDynamicRHI::FlushDeferredDeleteQueue()
	{
		// rhi thread
		uint32 index = (GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) + 1) % MAX_FRAME_LAG;
		DeferredDeleteQueue[index].clear();
	}

	void IDynamicRHI::SetMainViewportResolution_GameThread(TVector2u resolution)
	{
		uint32 const frameIndex = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) % 3;
		MainViewportResolution[frameIndex] = resolution;
	}

	TVector2u IDynamicRHI::GetMainViewportResolution_GameThread() const
	{
		uint32 const frameIndex = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) % 3;
		return MainViewportResolution[frameIndex];
	}

	TVector2u IDynamicRHI::GetMainViewportResolution_RenderThread() const
	{
		uint32 const frameIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 3;
		return MainViewportResolution[frameIndex];
	}

	TVector2u IDynamicRHI::GetMainViewportResolution_RHIThread() const
	{
		uint32 const frameIndex = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire) % 3;
		return MainViewportResolution[frameIndex];
	}
}
