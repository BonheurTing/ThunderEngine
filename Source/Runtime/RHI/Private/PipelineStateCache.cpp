#include "PipelineStateCache.h"
#include "IDynamicRHI.h"

namespace Thunder
{
    void SetGraphicsPipelineState(RenderContext* context, TGraphicsPipelineStateDescriptor& Initializer)
    {
        auto pipelineStateObject = RHICreateGraphicsPipelineState(Initializer);
    }
}
