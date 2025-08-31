#include "RenderResource.h"

namespace Thunder
{
	void RenderResource::InitResource()
	{
		//check(IsInRenderingThread());
		InitRHI();
	}

	void RenderResource::ReleaseResource()
	{
		//check(IsInRenderingThread());
		ReleaseRHI();
	}

	void RenderResource::UpdateRHI()
	{
		//check(IsInRenderingThread());
		ReleaseRHI();
		InitRHI();
	}
}
