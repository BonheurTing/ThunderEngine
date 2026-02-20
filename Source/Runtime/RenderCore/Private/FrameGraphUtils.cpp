#include "FrameGraphUtils.h"

#include "FrameGraph.h"
#include "Vector.h"

namespace Thunder
{
    void AddClearRenderTargetPass(TRefCountPtr<FrameGraph> GraphBuilder, TRefCountPtr<FGRenderTarget> Texture, const TVector4f& ClearColor)
    {
        PassOperations operations;
        operations.Write(Texture, ClearColor);
        GraphBuilder->AddPass(EVENT_NAME("ClearRenderTargetPass"), std::move(operations), []() {});
    }
}
