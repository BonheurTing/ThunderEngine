#include "RenderContext.h"
#include "RHICommand.h"
#include "Memory/TransientAllocator.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
    FRenderContext::FRenderContext()
    {
        // Reserve some initial capacity to avoid early reallocations
        Commands.reserve(1024);
        TransientAllocatorPtr[0] = new TransientAllocator();
        TransientAllocatorPtr[1] = new TransientAllocator();
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

    void FRenderContext::ClearCommands()
    {
        Commands.clear();
    }

    TransientAllocator* FRenderContext::GetTransientAllocator_RenderThread() const
    {
        uint32 index = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
        return TransientAllocatorPtr[index];
    }

    TransientAllocator* FRenderContext::GetTransientAllocator_RHIThread() const
    {
        uint32 index = GFrameState->FrameNumberRHIThread.load(std::memory_order_acquire) % 2;
        return TransientAllocatorPtr[index];
    }
}
