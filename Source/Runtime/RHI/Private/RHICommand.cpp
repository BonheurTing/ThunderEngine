
#pragma optimize("", off)
#include "RHICommand.h"

#include <d3d12.h>

#include "ShaderArchive.h"

namespace Thunder
{
    void RHIDummyCommand::Execute(RHICommandContext* cmdList)
    {
        LOG("execute dummy command");
    }

    void RHIDrawCommand::Execute(RHICommandContext* cmdList)
    {
        LOG("Execute draw command.");

        // Set pipeline state.
        if (GraphicsPSO)
        {
            cmdList->SetPipelineState(GraphicsPSO);
        }

        // Set bindings.
        constexpr uint32 kMaxSRVCount = MAX_SRVS;
        SingleShaderBindings* bindings = Bindings.GetSingleShaderBindings();
        ShaderBindingsLayout* bindingsLayout = Shader->GetSubShader()->GetArchive()->GetBindingsLayout();
        auto const& srvMap = bindingsLayout->GetSRVsIndexMap();
        D3D12_CPU_DESCRIPTOR_HANDLE srvTable[kMaxSRVCount] = {}; // Zero initialized.
        for (const auto& srvSlot : srvMap | std::views::keys)
        {
            if (srvSlot >= kMaxSRVCount)
            {
                TAssertf(false, "Max SRV count(%u) is exceeded, binding skipped.", kMaxSRVCount);
                continue;
            }
            uint64 srvHandle = bindings->GetSRV(bindingsLayout, srvSlot).Handle;
            srvTable[srvSlot] = { .ptr = srvHandle };
        }

        return;

        // Set vertex buffer
        if (VBToSet)
        {
            cmdList->SetVertexBuffer(0, 1, VBToSet);
        }

        // Set index buffer
        if (IBToSet)
        {
            cmdList->SetIndexBuffer(IBToSet);
        }

        // Execute the draw call
        // This will be expanded based on DrawType
        if (IBToSet)
        {
            cmdList->DrawIndexedInstanced(IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        }
        else
        {
            cmdList->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
        }
    }
}
