#pragma once

#include "CoreMinimal.h"
#include "RenderCore.export.h"
#include "RenderResource.h"
#include "RHI.h"
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace Thunder
{
    enum class EPixelFormat : uint8
    {
        UNKNOWN,
        RGBA8888,
        RGBA16F,
        RGBA32F,
        RGB10A2,
        R16F,
        R32F,
        D24S8,
        D32F
    };

    struct FGRenderTargetDesc
    {
        uint32 Width = 0;
        uint32 Height = 0;
        EPixelFormat Format = EPixelFormat::UNKNOWN;
        bool bIsDepthStencil = false;

        FGRenderTargetDesc() = default;
        FGRenderTargetDesc(uint32 InWidth, uint32 InHeight, EPixelFormat InFormat, bool bInIsDepthStencil = false)
            : Width(InWidth), Height(InHeight), Format(InFormat), bIsDepthStencil(bInIsDepthStencil) {}

        bool operator==(const FGRenderTargetDesc& Other) const
        {
            return Width == Other.Width && Height == Other.Height &&
                   Format == Other.Format && bIsDepthStencil == Other.bIsDepthStencil;
        }
    };

    class RENDERCORE_API FGRenderTarget
    {
    public:
        FGRenderTarget() = default;
        FGRenderTarget(uint32 Width, uint32 Height, EPixelFormat Format);

        const FGRenderTargetDesc& GetDesc() const { return Desc; }
        uint32 GetWidth() const { return Desc.Width; }
        uint32 GetHeight() const { return Desc.Height; }
        EPixelFormat GetFormat() const { return Desc.Format; }

        uint32 GetID() const { return ID; }

    private:
        FGRenderTargetDesc Desc;
        uint32 ID = 0;
        static uint32 NextID;
    };

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

    class RENDERCORE_API RenderTargetPool
    {
    public:
        RenderTextureRef AcquireRenderTarget(const FGRenderTargetDesc& Desc);
        void ReleaseRenderTarget(RenderTextureRef RenderTarget);
        void Reset();

    private:
        TArray<RenderTextureRef> AvailableTargets;
        TArray<RenderTextureRef> UsedTargets;
    };

    class RENDERCORE_API FrameGraph
    {
    public:
        FrameGraph();
        ~FrameGraph();

        // Delete copy constructor and copy assignment operator
        FrameGraph(const FrameGraph&) = delete;
        FrameGraph& operator=(const FrameGraph&) = delete;

        // Allow move constructor and move assignment operator
        FrameGraph(FrameGraph&&) = default;
        FrameGraph& operator=(FrameGraph&&) = default;

        void Setup();
        void Compile();
        void Execute();
        void Reset();

        // Get allocated render target for a given ID
        RenderTextureRef GetAllocatedRenderTarget(uint32 RenderTargetID) const;

        // Internal methods used by FrameGraphBuilder
        void AddPass(const String& Name, PassOperations&& Operations, PassExecutionFunction&& ExecuteFunction);
        void SetPresentTarget(const FGRenderTarget& RenderTarget);
        void RegisterRenderTarget(const FGRenderTarget& RenderTarget);

    private:
        void CullUnusedPasses();
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

        void AddPass(const String& Name, PassOperations&& Operations, PassExecutionFunction&& ExecuteFunction);
        void Present(const FGRenderTarget& RenderTarget);

    private:
        FrameGraph* FrameGraphPtr;
    };

    #define EVENT_NAME(Name) Name
}