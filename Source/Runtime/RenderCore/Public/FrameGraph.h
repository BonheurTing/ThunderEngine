#pragma once

#include "CoreMinimal.h"
#include "RenderResource.h"
#include "RenderTargetPool.h"
#include "RHI.h"

namespace Thunder
{

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

    using PassExecutionFunction = TFunction<void()>;

    struct PassData : public RefCountedObject
    {
        String Name;
        PassOperations Operations;
        PassExecutionFunction ExecuteFunction;
        bool bCulled = false;

        PassData(const String& InName, PassOperations&& InOperations, PassExecutionFunction&& InExecuteFunction)
            : Name(InName), Operations(std::move(InOperations)), ExecuteFunction(std::move(InExecuteFunction)) {}
    };

    class RENDERCORE_API FrameGraph : public RefCountedObject
    {
    public:
        void Setup();
        void Compile();
        void Execute();
        void Reset();

        // Internal methods used by FrameGraphBuilder
        void AddPass(const String& Name, PassOperations&& Operations, PassExecutionFunction&& ExecuteFunction);
        void SetPresentTarget(const FGRenderTarget& RenderTarget);
        void RegisterRenderTarget(const FGRenderTarget& RenderTarget);
        void ClearRenderTargetPool();

    private:
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
    };

    class RENDERCORE_API FrameGraphBuilder
    {
    public:
        explicit FrameGraphBuilder(FrameGraph* InFrameGraph);

        void AddPass(const String& Name, PassOperations&& Operations, PassExecutionFunction&& ExecuteFunction) const;
        void Present(const FGRenderTarget& RenderTarget) const;

    private:
        FrameGraph* FrameGraphPtr;
    };

    #define EVENT_NAME(Name) Name
}