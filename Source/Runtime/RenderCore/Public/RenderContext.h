#pragma once

#include "RenderCore.export.h"
#include "CoreMinimal.h"
#include "RHI.h"
#include "Memory/TransientAllocator.h"

namespace Thunder
{
    class IRHICommand;

    /**
     * Thread-specific render context that holds recorded commands
     * Each thread has its own context to record commands into
     */
    struct RENDERCORE_API FRenderContext
    {
    public:
        FRenderContext() = delete;
        FRenderContext(class FrameGraph* owner);
        ~FRenderContext();

        // Clear all commands and free allocator
        void FreeAllocator() const;

        // Add a command to this context
        void AddCommand(IRHICommand* Command);

        // Get all commands recorded in this context
        const TArray<IRHICommand*>& GetCommands() const { return Commands; }

        // Clear all commands and free allocator
        void ClearCommands();

        // Get the transient allocator for this context
        TransientAllocator* GetTransientAllocator_RenderThread() const;
        // TransientAllocator* GetTransientAllocator_RHIThread() const;

        template<typename T>
        void* Allocate(size_t count = 1) const // Call on render thread.
        {
            return GetTransientAllocator_RenderThread()->Allocate(sizeof(T) * count, alignof(T));
        }

        FORCEINLINE void SetCurrentPass(class FrameGraphPass* pass) { CurrentPass = pass; }
        FORCEINLINE FrameGraphPass* GetCurrentPass() const { return CurrentPass; }
        FORCEINLINE FrameGraph* GetFrameGraph() const { return FrameGraph; }
        RenderPass* GetRenderPass() const;

    private:
        // Array of recorded commands
        TArray<IRHICommand*> Commands;

        // Transient allocator for command allocation
        TransientAllocator* TransientAllocatorPtr[2];
        
        FrameGraph* FrameGraph = nullptr;
        FrameGraphPass* CurrentPass = nullptr;
    };
}
