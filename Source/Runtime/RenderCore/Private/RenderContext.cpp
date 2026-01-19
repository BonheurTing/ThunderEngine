#include "RenderContext.h"

#include "FrameGraph.h"
#include "RenderModule.h"
#include "RHICommand.h"
#include "Memory/TransientAllocator.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
    FRenderContext::FRenderContext(class FrameGraph* owner)
    {
        // Reserve some initial capacity to avoid early reallocations
        Commands.reserve(1024);
        TransientAllocatorPtr[0] = new TransientAllocator();
        TransientAllocatorPtr[1] = new TransientAllocator();
        FrameGraph = owner;
    }

    FRenderContext::~FRenderContext()
    {
        ClearCommands();
        delete TransientAllocatorPtr;
    }

    void FRenderContext::FreeAllocator() const
    {
        GetTransientAllocator_RenderThread()->FreeAll();
    }

    void FRenderContext::AddCommand(IRHICommand* Command)
    {
        if (Command)
        {
            Commands.push_back(Command);
        }
    }

    void FRenderContext::AddCommandList(TArray<RHICachedDrawCommand*> commandList)
    {
        Commands.insert(Commands.end(), commandList.begin(), commandList.end());
    }

    void FRenderContext::AddCachedCommand(const MeshBatch* batch, RHICachedDrawCommand* command)
    {
        if (batch && command)
        {
            CachedDrawCommands.emplace_back(batch, command);
        }
        else
        {
            TAssertf(false, "Failed to add cached draw command, batch or command is invalid.");
        }
    }

    void FRenderContext::ClearCachedCommands()
    {
        CachedDrawCommands.clear();
    }

    void FRenderContext::ClearCommands()
    {
        Commands.clear();
    }

    TransientAllocator* FRenderContext::GetTransientAllocator_RenderThread() const
    {
        uint32 index = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
        return TransientAllocatorPtr[index];
    }

    // TransientAllocator* FRenderContext::GetTransientAllocator_RHIThread() const
    // {
    //     uint32 index = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire) % 2;
    //     return TransientAllocatorPtr[index];
    // }

    RenderPass* FRenderContext::GetRenderPass() const
    {
        RenderPassKey key;
        auto const& renderTargetIndices = CurrentPass->GetOperations().GetWriteTargets();
        TAssertf(renderTargetIndices.size() <= 8, "Rendering into more than 8 render targets is not supported.");
        key.RenderTargetCount = static_cast<uint8>(renderTargetIndices.size());
        uint32 outTargetIndex = 0;
        for (auto const& targetIndex : renderTargetIndices)
        {
            RHIFormat format = RHIFormat::UNKNOWN;
            bool isDepthStencil = false;
            bool succeeded = FrameGraph->GetRenderTargetFormat(targetIndex, format, isDepthStencil);
            if (!succeeded) [[unlikely]]
            {
                TAssertf(false, "Failed to get render target format, target index : %u.", targetIndex);
                return nullptr;
            }

            if (isDepthStencil)
            {
                key.DepthStencilFormat = static_cast<uint8>(format);
                key.RenderTargetCount -= 1; // Last render target is the depth target.
            }
            else
            {
                key.RenderTargetFormats[outTargetIndex] = static_cast<uint8>(format);
                ++outTargetIndex;
            }
        }

        return RenderModule::GetRenderPass(key);
    }
}
