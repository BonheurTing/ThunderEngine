
#pragma optimize("", off)
#include "RHICommand.h"
#include <algorithm>
#include "ShaderArchive.h"
#include "UniformBuffer.h"

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
        BindPSO(cmdList);

        // Build and set SRV table.
        BindSRVTable(cmdList);

        BindCBV(cmdList);

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
        if (IBToSet)
        {
            cmdList->DrawIndexedInstanced(IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        }
        else
        {
            cmdList->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
        }
    }

    void RHIDrawCommand::BindPSO(RHICommandContext* cmdList) const
    {
        if (GraphicsPSO)
        {
            cmdList->SetPipelineState(GraphicsPSO);
        }
    }

    void RHIDrawCommand::BindSRVTable(RHICommandContext* cmdList)
    {
        // Get bindings.
        constexpr uint32 kMaxSRVCount = MAX_SRVS;
        SingleShaderBindings* bindings = Bindings.GetSingleShaderBindings();
        ShaderBindingsLayout* bindingsLayout = Shader->GetSubShader()->GetArchive()->GetBindingsLayout();
        auto const& srvMap = bindingsLayout->GetSRVsIndexMap();

        // Prepare SRV handle array.
        uint64 srvHandles[kMaxSRVCount] = {}; // Zero initialized.
        uint32 maxSlotUsedCount = 0;
        for (const auto& srvSlot : srvMap | std::views::keys)
        {
            if (srvSlot >= kMaxSRVCount)
            {
                TAssertf(false, "SRV slot %u exceeds max count %u, binding skipped.", srvSlot, kMaxSRVCount);
                continue;
            }

            // Get the offline descriptor handle from bindings.
            uint64 srvHandle = bindings->GetSRV(bindingsLayout, srvSlot).Handle;
            srvHandles[srvSlot] = srvHandle;
            maxSlotUsedCount = std::max<uint32>(srvSlot + 1, maxSlotUsedCount);
        }

        // Bind SRVs.
        if (maxSlotUsedCount > 0)
        {
            uint32 srvCount = maxSlotUsedCount;
            TShaderRegisterCounts const& shaderRC = Shader->GetSubShader()->GetShaderRegisterCounts();
            cmdList->BindSRVTable(shaderRC, srvHandles, srvCount);
        }
    }

    void RHIDrawCommand::BindCBV(RHICommandContext* cmdList)
    {
        // Get bindings.
        constexpr uint32 kMaxCBVCount = MAX_CBS;
        SingleShaderBindings* bindings = Bindings.GetSingleShaderBindings();
        ShaderBindingsLayout* bindingsLayout = Shader->GetSubShader()->GetArchive()->GetBindingsLayout();
        auto const& cbvMap = bindingsLayout->GetUniformBuffersIndexMap();

        // Prepare GPU virtual address array for root descriptor binding.
        uint64 cbvGpuAddresses[kMaxCBVCount] = {}; // Zero initialized.
        uint32 maxSlotUsedCount = 0;
        for (const auto& cbvSlot : cbvMap | std::views::keys)
        {
            if (cbvSlot >= kMaxCBVCount)
            {
                TAssertf(false, "CBV slot %u exceeds max count %u, binding skipped.", cbvSlot, kMaxCBVCount);
                continue;
            }

            // Get the uniform buffer from bindings.
            uint64 ubRawPtr = bindings->GetUniformBuffer(bindingsLayout, cbvSlot).Handle;
            RHIUniformBuffer* uniformBuffer = reinterpret_cast<RHIUniformBuffer*>(ubRawPtr);
            if (uniformBuffer == nullptr) [[unlikely]]
            {
                TAssertf(false, "CBV slot %u: uniform buffer is invalid.", cbvSlot);
                cbvGpuAddresses[cbvSlot] = 0;
                continue;
            }

            // Get the GPU virtual address of the underlying D3D12 resource.
            cbvGpuAddresses[cbvSlot] = uniformBuffer->GetGpuVirtualAddress();
            maxSlotUsedCount = std::max<uint32>(cbvSlot + 1, maxSlotUsedCount);
        }

        // Bind CBVs as root descriptors.
        if (maxSlotUsedCount > 0)
        {
            uint32 cbvCount = maxSlotUsedCount;
            TShaderRegisterCounts const& shaderRC = Shader->GetSubShader()->GetShaderRegisterCounts();
            cmdList->BindCBVs(shaderRC, cbvGpuAddresses, cbvCount);
        }
    }

    void RHIBeginPassCommand::Execute(RHICommandContext* cmdList)
    {
        TArray<RHIRenderTargetView*> rtvPtrs(RenderTargetCount);
        for (uint32 i = 0; i < RenderTargetCount; i++)
        {
            rtvPtrs[i] = RenderTargets[i]->GetRTV();
        }
        RHIDepthStencilView* dsv = nullptr;
        if (DepthStencil.IsValid())
        {
            dsv = DepthStencil->GetDSV();
        }
        // Bind.
        cmdList->SetRenderTarget(RenderTargetCount, rtvPtrs, dsv);
    }
}
