
#pragma optimize("", off)
#include "RHICommand.h"

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
        SingleShaderBindings* bindings = Bindings.GetSingleShaderBindings();

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
