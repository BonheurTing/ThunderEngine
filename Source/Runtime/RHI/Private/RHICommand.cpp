
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

        // Build and set SRV table
        /*
        // Cast to D3D12 types
        auto* d3d12Context = static_cast<D3D12CommandContext*>(cmdList);
        auto* d3d12Device = d3d12Context->GetDevice().Get();
        auto* descriptorCache = d3d12Context->GetDescriptorCache();
        auto* commandList = d3d12Context->GetCommandList().Get();

        constexpr uint32 kMaxSRVCount = MAX_SRVS;
        SingleShaderBindings* bindings = Bindings.GetSingleShaderBindings();
        ShaderBindingsLayout* bindingsLayout = Shader->GetSubShader()->GetArchive()->GetBindingsLayout();
        auto const& srvMap = bindingsLayout->GetSRVsIndexMap();

        if (!srvMap.empty())
        {
            // Determine the number of SRVs to bind
            uint32 numSRVs = 0;
            for (const auto& [srvSlot, info] : srvMap)
            {
                numSRVs = (srvSlot + 1) > numSRVs ? (srvSlot + 1) : numSRVs;
            }
            numSRVs = numSRVs > kMaxSRVCount ? kMaxSRVCount : numSRVs;

            if (numSRVs > 0)
            {
                // Reserve slots from the online heap
                auto* currentHeap = descriptorCache->GetCurrentViewHeap();
                uint32 baseSlot = currentHeap->ReserveSlots(numSRVs);

                // Prepare source and destination descriptors for CopyDescriptors
                D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors[kMaxSRVCount] = {};
                D3D12_CPU_DESCRIPTOR_HANDLE dstDescriptors[kMaxSRVCount] = {};
                uint32 validDescriptorCount = 0;

                for (const auto& [srvSlot, info] : srvMap)
                {
                    if (srvSlot >= kMaxSRVCount)
                    {
                        TAssertf(false, "Max SRV count(%u) is exceeded, binding skipped.", kMaxSRVCount);
                        continue;
                    }

                    // Get the source SRV handle from bindings
                    uint64 srvHandle = bindings->GetSRV(bindingsLayout, srvSlot).Handle;
                    if (srvHandle != 0xFFFFFFFFFFFFFFFF) // Check if valid
                    {
                        srcDescriptors[srvSlot] = { .ptr = srvHandle };
                        dstDescriptors[srvSlot] = currentHeap->GetCPUSlotHandle(baseSlot + srvSlot);
                        validDescriptorCount++;
                    }
                }

                // Copy descriptors from offline heap to online heap
                if (validDescriptorCount > 0)
                {
                    d3d12Device->CopyDescriptors(
                        numSRVs, dstDescriptors, nullptr,
                        numSRVs, srcDescriptors, nullptr,
                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
                    );

                    // Set the descriptor table on the command list
                    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = currentHeap->GetGPUSlotHandle(baseSlot);
                    // TODO: Set root parameter index based on root signature design
                    // For now, assuming SRV table is at root parameter index 0
                    constexpr uint32 kSRVTableRootParameterIndex = 0;
                    commandList->SetGraphicsRootDescriptorTable(kSRVTableRootParameterIndex, gpuHandle);

                    LOG("Set SRV table: numSRVs=%u, baseSlot=%u, gpuHandle=%llu",
                        numSRVs, baseSlot, gpuHandle.ptr);
                }
            }
        }
        */
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
