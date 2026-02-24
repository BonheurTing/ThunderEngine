#include "RenderContext.h"

#include "FrameGraph.h"
#include "RenderModule.h"
#include "RHICommand.h"
#include "Memory/TransientAllocator.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
    RenderContext::RenderContext(class FrameGraph* owner)
    {
        // Reserve some initial capacity to avoid early reallocations
        Commands.reserve(1024);
        TransientAllocatorPtr[0] = new TransientAllocator();
        TransientAllocatorPtr[1] = new TransientAllocator();
        FrameGraph = owner;
    }

    RenderContext::~RenderContext()
    {
        ClearCommands();
        delete TransientAllocatorPtr[0];
        delete TransientAllocatorPtr[1];
    }

    void RenderContext::FreeAllocator() const
    {
        GetTransientAllocator_RenderThread()->FreeAll();
    }

    void RenderContext::AddCommand(IRHICommand* Command)
    {
        if (Command)
        {
            Commands.push_back(Command);
        }
    }

    void RenderContext::AddCommandList(TArray<RHICachedDrawCommand*> commandList)
    {
        Commands.insert(Commands.end(), commandList.begin(), commandList.end());
    }

    void RenderContext::AddCachedCommand(const MeshBatch* batch, RHICachedDrawCommand* command)
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

    void RenderContext::ClearCachedCommands()
    {
        CachedDrawCommands.clear();
    }

    void RenderContext::ClearCommands()
    {
        Commands.clear();
    }

    TransientAllocator* RenderContext::GetTransientAllocator_RenderThread() const
    {
        uint32 index = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
        return TransientAllocatorPtr[index];
    }

    // TransientAllocator* FRenderContext::GetTransientAllocator_RHIThread() const
    // {
    //     uint32 index = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire) % 2;
    //     return TransientAllocatorPtr[index];
    // }

    RenderPass* RenderContext::GetRenderPass() const
    {
        RenderPassKey key;
        auto const& renderTargetIndices = CurrentPass->GetOperations().GetWriteTargets();
        TAssertf(renderTargetIndices.size() <= 8, "Rendering into more than 8 render targets is not supported.");
        if (CurrentPass->bIsPresentPass)
        {
            key.RenderTargetCount = 1;
            key.RenderTargetFormats[0] = static_cast<uint8>(RHIFormat::R8G8B8A8_UNORM);
        }
        else
        {
            key.RenderTargetCount = static_cast<uint8>(renderTargetIndices.size());
            uint32 outTargetIndex = 0;
            for (auto const& targetIndex : renderTargetIndices | std::views::keys)
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
        }

        return RenderModule::GetRenderPass(key);
    }
}
