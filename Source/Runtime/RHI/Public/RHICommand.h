#pragma once
#include "IDynamicRHI.h"
#include "RHIContext.h"
#include "RHIResource.h"
#include "ShaderBindings.h"
#include "Memory/TransientAllocator.h"

namespace Thunder
{
    class RHICommandContext;
    class TransientAllocator;
    struct IRHICommand;

    /**
     * Abstract interface for recording RHI commands.
     * Lives in the RHI module so that RHI backends (D3D12RHI, etc.) can
     * accept a command recorder without depending on higher-level modules
     * like RenderCore where the concrete RenderContext is defined.
     */
    struct RHI_API IRHICommandRecorder
    {
        virtual ~IRHICommandRecorder() = default;

        /** Allocate raw memory from the transient allocator. */
        virtual TransientAllocator* GetTransientAllocator_RenderThread() const = 0;

        /** Enqueue a command into this recorder. */
        virtual void AddCommand(IRHICommand* command) = 0;

        /** Typed allocation helper. */
        template<typename T>
        void* Allocate(size_t count = 1) const
        {
            return GetTransientAllocator_RenderThread()->Allocate(sizeof(T) * count, alignof(T));
        }
    };

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

    constexpr uint32 kMaxRTVCount = MAX_RTVS;
    struct RHIBeginPassCommand : public IRHICommand
    {
        RHI_API void Execute(RHICommandContext* cmdList) override;

        RHITextureRef RenderTargets[kMaxRTVCount];
        RHITextureRef DepthStencil;
        uint32 RenderTargetCount = 0;
    };

    struct RHIEndPassCommand : public IRHICommand
    {
        RHI_API void Execute(RHICommandContext* cmdList) override {}
    };
}
