#pragma once
#include "IDynamicRHI.h"
#include "RHIContext.h"
#include "RHIResource.h"
#include "ShaderBindings.h"

namespace Thunder
{
    class RHICommandContext;
    class TransientAllocator;

    /**
     * Abstract base class for all RHI commands
     */
    struct IRHICommand
    {
        virtual ~IRHICommand() = default;

        /** Execute and then destruct the command */
        FORCEINLINE void ExecuteAndDestruct(RHICommandContext* cmdList)
        {
            Execute(cmdList);
            if (!IsCached)
            {
                this->~IRHICommand();
            }
        }
        /** Execute the command on the given command context */
        RHI_API virtual void Execute(RHICommandContext* cmdList) = 0;

        bool IsCached = false;
    };

    /**
     * Simple vertex buffer setting command
     */
    struct RHIDummyCommand : public IRHICommand
    {
        RHI_API void Execute(RHICommandContext* cmdList) override;
    };

    /**
     * Complete draw command that includes all necessary render state
     * This is a consolidated draw command rather than many small commands like UE
     */
    struct RHIDrawCommand : public IRHICommand
    {
        RHI_API void Execute(RHICommandContext* cmdList) override;
        void BindPSO(RHICommandContext* cmdList) const;
        void BindSRVTable(RHICommandContext* cmdList);
        void BindCBV(RHICommandContext* cmdList);

        // Render resources.
        RHIVertexBuffer* VBToSet = nullptr;
        RHIIndexBuffer* IBToSet = nullptr;
        TRHIPipelineState* GraphicsPSO = nullptr;
        ShaderCombination* Shader = nullptr;
        ShaderBindings Bindings{};

        // TODO.
        // RHIConstantBufferRef CB;
        // TArray<RHIResourceBinding> Bindings;
        // ERHIDrawType DrawType;

        // Draw parameters.
        uint32 IndexCount = 0;
        uint32 VertexCount = 0;
        uint32 InstanceCount = 1;
        uint32 StartIndexLocation = 0;
        int32 BaseVertexLocation = 0;
        uint32 StartInstanceLocation = 0;
        uint32 StartVertexLocation = 0;
    };

    struct RHICachedDrawCommand : public RHIDrawCommand
    {
        RHICachedDrawCommand() : CachedCommandIndex(RHIAllocateCachedMeshDrawCommandIndex()) { IsCached = true; }
        uint64 CachedCommandIndex = 0;
    };
}
