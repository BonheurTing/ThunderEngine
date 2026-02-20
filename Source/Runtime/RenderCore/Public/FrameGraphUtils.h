#pragma once
#include "Vector.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
    class FrameGraph;
    class FGRenderTarget;

    RENDERCORE_API void AddClearRenderTargetPass(TRefCountPtr<FrameGraph> GraphBuilder, TRefCountPtr<FGRenderTarget> Texture, const TVector4f& ClearColor);
}
