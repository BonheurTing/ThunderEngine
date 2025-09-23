#include "RenderResource.h"

namespace Thunder
{
	TArray<RHIResource*> GRHIUpdateSyncQueue {};
	TArray<RHIResource*> GRHIUpdateAsyncQueue {};

	void RenderResource::InitResource()
	{
		//check(IsInRenderingThread());
		// if GIsRHIInitialized
		InitRHI();
	}

	void RenderResource::ReleaseResource()
	{
		//check(IsInRenderingThread());
		// if GIsRHIInitialized
		ReleaseRHI();
	}

	void RenderResource::UpdateRHI()
	{
		//check(IsInRenderingThread());
		ReleaseRHI();
		InitRHI();
	}

}
