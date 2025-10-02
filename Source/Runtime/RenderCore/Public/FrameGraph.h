#pragma once

#include "CoreMinimal.h"
#include "RenderResource.h"
#include "RenderTargetPool.h"
#include "RenderContext.h"
#include "RHI.h"

namespace Thunder
{
    class FRenderContext;
    class IRHICommand;

    class RENDERCORE_API PassOperations
    {
    public:
        void read(const FGRenderTarget& RenderTarget);
        void write(const FGRenderTarget& RenderTarget);

        const TArray<uint32>& GetReadTargets() const { return ReadTargets; }
        const TArray<uint32>& GetWriteTargets() const { return WriteTargets; }

    private:
        TArray<uint32> ReadTargets;
        TArray<uint32> WriteTargets;
    };

    using PassExecutionFunction = TFunction<void(FRenderContext*)>;

    struct PassData : public RefCountedObject
    {
        String Name;
        PassOperations Operations;
        PassExecutionFunction ExecuteFunction;
        bool bCulled = false;
        bool bIsMeshDrawPass = false;  // Flag to indicate if this is a mesh draw pass

        PassData(const String& InName, PassOperations&& InOperations, PassExecutionFunction&& InExecuteFunction, bool InIsMeshDrawPass = false)
            : Name(InName), Operations(std::move(InOperations)), ExecuteFunction(std::move(InExecuteFunction)), bIsMeshDrawPass(InIsMeshDrawPass) {}
    };

    class RENDERCORE_API FrameGraph : public RefCountedObject
    {
    public:
        FrameGraph(int contextNum)
        {
            InitializeRenderContexts(contextNum);
        }

        void Reset();
        void Compile();
        void Execute();

        // Internal methods used by FrameGraphBuilder
        void AddPass(const String& Name, PassOperations&& Operations, PassExecutionFunction&& ExecuteFunction, bool bIsMeshDrawPass = false);
        void SetPresentTarget(const FGRenderTarget& RenderTarget);
        void RegisterRenderTarget(const FGRenderTarget& RenderTarget);
        void ClearRenderTargetPool();

        // Command execution support
        FRenderContext* GetMainContext() const { return MainContext; }
        const TArray<FRenderContext*>& GetRenderContexts() const { return RenderContexts; }
        TArray<IRHICommand*>& GetCurrentAllCommands(int frameIndex) { return AllCommands[frameIndex]; }

    private:
        // Initialize render contexts for multi-threading
        void InitializeRenderContexts(uint32 threadCount);

        void CullUnusedPasses() const;
        void TopologicalSort();
        void ScheduleRenderTargetLifetime();
        void AllocateRenderTargets();

        TArray<TRefCountPtr<PassData>> Passes;
        TArray<size_t> ExecutionOrder;
        THashMap<uint32, FGRenderTargetDesc> RenderTargetDescs;
        THashMap<uint32, RenderTextureRef> AllocatedRenderTargets;
        THashMap<uint32, std::pair<size_t, size_t>> RenderTargetLifetimes; // target -> (first_use, last_use)

        uint32 PresentTargetID = 0;
        bool bHasPresentTarget = false;

        RenderTargetPool Pool;

        // Command execution contexts
        FRenderContext* MainContext = nullptr;  // Main render thread context
        TArray<FRenderContext*> RenderContexts; // Worker thread contexts
        TArray<IRHICommand*> AllCommands[2];       // Consolidated commands for execution
    };

    #define EVENT_NAME(Name) Name
}