#pragma once

#include "RenderCore.export.h"
#include "CoreMinimal.h"
#include "RHI.h"

namespace Thunder
{
    class IRHICommand;
    class TransientAllocator;

    /**
     * Thread-specific render context that holds recorded commands
     * Each thread has its own context to record commands into
     */
    struct RENDERCORE_API FRenderContext
    {
    public:
        FRenderContext();
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
        TransientAllocator* GetTransientAllocator_RHIThread() const;

    private:
        // Array of recorded commands
        TArray<IRHICommand*> Commands;

        // Transient allocator for command allocation
        TransientAllocator* TransientAllocatorPtr[2];
    };
}