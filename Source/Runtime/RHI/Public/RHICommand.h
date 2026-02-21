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

    struct RHIBeginFrameCommand : public IRHICommand
    {
        RHI_API void Execute(RHICommandContext* cmdList) override;
    };

    struct RHIBeginCommandListCommand : public IRHICommand
    {
        RHI_API void Execute(RHICommandContext* cmdList) override;

        RHIBeginCommandListCommand(struct RHIPassState* state) : PassState(state) {}

        RHIPassState* PassState;
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
    };

    struct RHICachedDrawCommand : public RHIDrawCommand
    {
        RHICachedDrawCommand() : CachedCommandIndex(RHIAllocateCachedMeshDrawCommandIndex()) { IsCached = true; }
        uint64 CachedCommandIndex = 0;
    };

    enum class ELoadOp : uint8
    {
        Load,     // Preserve existing contents
        Clear,    // Clear to the specified clear value
        DontCare, // Contents are undefined; driver may do anything
    };

    struct TReadTextureBinding
    {
        RHITextureRef Texture;
        bool bNeedBarrier = false;
        ERHIResourceState OldState;
    };

    constexpr uint32 kMaxRTVCount = MAX_RTVS;
    struct RHIBeginPassCommand : public IRHICommand
    {
        RHI_API void Execute(RHICommandContext* cmdList) override;

        struct TRenderTargetBinding
        {
            RHITextureRef Texture;
            ELoadOp       LoadOp     = ELoadOp::Load;
            TVector4f     ClearColor = { 0.f, 0.f, 0.f, 1.f };
            bool bNeedBarrier = false;
            ERHIResourceState OldState;
        };

        struct TDepthStencilBinding
        {
            RHITextureRef Texture;
            ELoadOp       LoadOp       = ELoadOp::Load;
            float         ClearDepth   = 1.f;
            uint8         ClearStencil = 0;
            bool bNeedBarrier = false;
            ERHIResourceState OldState;
        };


        TRenderTargetBinding RenderTargets[kMaxRTVCount];
        TDepthStencilBinding DepthStencil;
        uint32 RenderTargetCount = 0;

        TArray<TReadTextureBinding> ReadRenderTargets;
        TReadTextureBinding ReadDepthStencil;

        // When true, this pass renders directly to the swapchain backbuffer.
        // No pool render targets are bound; the backbuffer is bound instead.
        bool bIsBackBufferPass = false;
    };

    struct RHIEndPassCommand : public IRHICommand
    {
        RHI_API void Execute(RHICommandContext* cmdList) override;

        // When true, transitions the swapchain backbuffer to Present state.
        bool bIsBackBufferPass = false;
    };

    struct RHIPassState
    {
        RHIPassState(const RHIBeginPassCommand* beginCommand, uint32 commandId);
        ~RHIPassState() = default;

        RHITextureRef RenderTargets[kMaxRTVCount];
        RHITextureRef DepthStencil;
        uint32 RenderTargetCount = 0;

        // When true, this pass renders to the swapchain backbuffer.
        // RHIBeginCommandListCommand will call SetBackBufferAsRenderTarget() instead of restoring pool RTs.
        bool bIsBackBufferPass = false;

        uint32 BeginCommandIndex = 0;
    };
}
