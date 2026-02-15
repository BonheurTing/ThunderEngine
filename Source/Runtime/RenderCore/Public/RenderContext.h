#pragma once

#include "RenderCore.export.h"
#include "CoreMinimal.h"
#include "MeshPass.h"
#include "RHI.h"
#include "RHICommand.h"

namespace Thunder
{
    /**
     * Thread-specific render context that holds recorded commands
     * Each thread has its own context to record commands into
     */
    struct RENDERCORE_API RenderContext : public IRHICommandRecorder
    {
    public:
        RenderContext() = delete;
        RenderContext(class FrameGraph* owner);
        ~RenderContext() override;

        // Clear all commands and free allocator
        void FreeAllocator() const;

        // IRHICommandRecorder interface
        void AddCommand(IRHICommand* Command) override;
        TransientAllocator* GetTransientAllocator_RenderThread() const override;

        // Add command list.
        void AddCommandList(TArray<struct RHICachedDrawCommand*> commandList);

        // Get all commands recorded in this context
        const TArray<IRHICommand*>& GetCommands() const { return Commands; }
        TArray<IRHICommand*>&& MoveCommands() { return std::move(Commands); }

        // Clear all commands and free allocator
        void ClearCommands();

        // Cache mesh-draw command.
        void AddCachedCommand(const class MeshBatch* batch, class RHICachedDrawCommand* command);
        TArray<std::tuple<const MeshBatch*, RHICachedDrawCommand*>> const& GetCachedCommands() const { return CachedDrawCommands; }
        void ClearCachedCommands();

        FORCEINLINE void SetCurrentPass(class FrameGraphPass* pass) { CurrentPass = pass; }
        FORCEINLINE FrameGraphPass* GetCurrentPass() const { return CurrentPass; }
        FORCEINLINE FrameGraph* GetFrameGraph() const { return FrameGraph; }
        RenderPass* GetRenderPass() const;

    private:
        // Array of recorded commands
        TArray<IRHICommand*> Commands;

        // Cached commands.
        TArray<std::tuple<const MeshBatch*, RHICachedDrawCommand*>> CachedDrawCommands;

        // Transient allocator for command allocation
        TransientAllocator* TransientAllocatorPtr[2];
        
        FrameGraph* FrameGraph = nullptr;
        FrameGraphPass* CurrentPass = nullptr;
    };
}
