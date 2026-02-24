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
    struct RenderContext : public IRHICommandRecorder
    {
    public:
        RENDERCORE_API RenderContext() = delete;
        RENDERCORE_API RenderContext(class FrameGraph* owner);
        RENDERCORE_API ~RenderContext() override;

        // Clear all commands and free allocator
        RENDERCORE_API void FreeAllocator() const;

        // IRHICommandRecorder interface
        RENDERCORE_API void AddCommand(IRHICommand* Command) override;
        RENDERCORE_API TransientAllocator* GetTransientAllocator_RenderThread() const override;

        // Add command list.
        RENDERCORE_API void AddCommandList(TArray<struct RHICachedDrawCommand*> commandList);

        // Get all commands recorded in this context
        FORCEINLINE const TArray<IRHICommand*>& GetCommands() const { return Commands; }
        FORCEINLINE TArray<IRHICommand*>&& MoveCommands() { return std::move(Commands); }

        // Clear all commands and free allocator
        RENDERCORE_API void ClearCommands();

        // Cache mesh-draw command.
        RENDERCORE_API void AddCachedCommand(const class MeshBatch* batch, class RHICachedDrawCommand* command);
        FORCEINLINE TArray<std::tuple<const MeshBatch*, RHICachedDrawCommand*>> const& GetCachedCommands() const { return CachedDrawCommands; }
        RENDERCORE_API void ClearCachedCommands();

        FORCEINLINE void SetCurrentPass(struct FrameGraphPass* pass) { CurrentPass = pass; }
        FORCEINLINE FrameGraphPass* GetCurrentPass() const { return CurrentPass; }
        FORCEINLINE FrameGraph* GetFrameGraph() const { return FrameGraph; }
        RENDERCORE_API RenderPass* GetRenderPass() const;

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
