#include "PipelineStateCache.h"
#include "IDynamicRHI.h"

namespace Thunder
{
    void SetGraphicsPipelineState(FRenderContext* context, TGraphicsPipelineStateDescriptor& Initializer)
    {
        auto pipelineStateObject = RHICreateGraphicsPipelineState(Initializer);
    }
}
