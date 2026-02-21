
#pragma optimize("", off)
#include "RHICommand.h"
#include <algorithm>
#include "ShaderArchive.h"
#include "UniformBuffer.h"

namespace Thunder
{
    void RHIDummyCommand::Execute(RHICommandContext* cmdList)
    {
    }

    void RHIBeginFrameCommand::Execute(RHICommandContext* cmdList)
    {
        cmdList->BeginFrame();
    }

    void RHIBeginCommandListCommand::Execute(RHICommandContext* cmdList)
    {
        uint32 const rtCount = PassState->RenderTargetCount;
        if (rtCount == 0)
        {
            return;
        }

        TArray<RHIRenderTargetView*> rtvPtrs(rtCount);
        for (uint32 i = 0; i < rtCount; i++)
        {
            rtvPtrs[i] = PassState->RenderTargets[i]->GetRTV();
        }
        RHIDepthStencilView* dsv = nullptr;
        if (PassState->DepthStencil.IsValid())
        {
            dsv = PassState->DepthStencil->GetDSV();
        }

        cmdList->SetRenderTarget(rtCount, rtvPtrs, dsv);
    }

    void RHIDrawCommand::Execute(RHICommandContext* cmdList)
    {
        // Set pipeline state.
        BindPSO(cmdList);

        // Build and set SRV table.
        BindSRVTable(cmdList);

        BindCBV(cmdList);

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
            cmdList->DrawInstanced(VertexCount, InstanceCount, BaseVertexLocation, StartInstanceLocation);
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
        for (auto& readRes : ReadRenderTargets)
        {
            if (!readRes.Texture.IsValid())
            {
                TAssertf(false, "Read RTs is invalid.");
                continue;
            }
            if (readRes.bNeedBarrier)
            {
                cmdList->TransitionBarrier(readRes.Texture.Get(), readRes.OldState, ERHIResourceState::AllShaderResource);
            }
        }
        if (ReadDepthStencil.Texture.IsValid())
        {
            if (ReadDepthStencil.bNeedBarrier)
            {
                cmdList->TransitionBarrier(ReadDepthStencil.Texture.Get(),ReadDepthStencil.OldState, ERHIResourceState::DepthRead);
            }
        }

        TArray<RHIRenderTargetView*> rtvPtrs(RenderTargetCount);
        for (uint32 i = 0; i < RenderTargetCount; i++)
        {
            if (RenderTargets[i].bNeedBarrier)
            {
                cmdList->TransitionBarrier(RenderTargets[i].Texture.Get(), RenderTargets[i].OldState, ERHIResourceState::RenderTarget);
            }
            rtvPtrs[i] = RenderTargets[i].Texture->GetRTV();
            if (RenderTargets[i].LoadOp == ELoadOp::Clear)
            {
                cmdList->ClearRenderTargetView(rtvPtrs[i], RenderTargets[i].ClearColor);
            }
        }
        RHIDepthStencilView* dsv = nullptr;
        if (DepthStencil.Texture.IsValid())
        {
            if (DepthStencil.bNeedBarrier)
            {
                cmdList->TransitionBarrier(DepthStencil.Texture.Get(), DepthStencil.OldState, ERHIResourceState::DepthWrite);
            }
            dsv = DepthStencil.Texture->GetDSV();
            if (DepthStencil.LoadOp == ELoadOp::Clear)
            {
                cmdList->ClearDepthStencilView(dsv, ERHIClearFlags::DepthStencil, DepthStencil.ClearDepth, DepthStencil.ClearStencil);
            }
        }

        cmdList->SetRenderTarget(RenderTargetCount, rtvPtrs, dsv);
    }

    void RHIEndPassCommand::Execute(RHICommandContext* cmdList)
    {
        if (!bPresentPass) return;
        if (!PresentTexture.Texture.IsValid())
        {
            TAssertf(false, "PresentTexture is invalid.");
            return;
        }
        if (PresentTexture.bNeedBarrier)
        {
            cmdList->TransitionBarrier(PresentTexture.Texture.Get(), PresentTexture.OldState, ERHIResourceState::Present);
        }
    }
}
