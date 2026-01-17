#pragma once

#include "CoreMinimal.h"
//#include "IRenderer.h"
#include "RenderResource.h"
#include "RenderTargetPool.h"
#include "RenderContext.h"
#include "RHI.h"
#include "SceneView.h"

namespace Thunder
{
    class FRenderContext;
    class IRHICommand;

    class RENDERCORE_API PassOperations
    {
    public:
        void read(const FGRenderTarget& renderTarget);
        void write(const FGRenderTarget& renderTarget);

        const TArray<uint32>& GetReadTargets() const { return ReadTargets; }
        const TArray<uint32>& GetWriteTargets() const { return WriteTargets; }

    private:
        TArray<uint32> ReadTargets;
        TArray<uint32> WriteTargets;
    };

    using PassExecutionFunction = TFunction<void(FRenderContext*)>;

    struct FrameGraphPass : public RefCountedObject
    {
        String Name;
        PassOperations Operations;
        PassExecutionFunction ExecuteFunction;
        bool bCulled = false;
        bool bIsMeshDrawPass = false;  // Flag to indicate if this is a mesh draw pass.
        TArray<FGRenderTarget> RenderTargets;

        FrameGraphPass() = delete;
        FrameGraphPass(const String& inName, PassOperations&& inOperations, PassExecutionFunction&& inExecuteFunction, bool inIsMeshDrawPass = false)
            : Name(inName), Operations(std::move(inOperations)), ExecuteFunction(std::move(inExecuteFunction)), bIsMeshDrawPass(inIsMeshDrawPass)
        {
            auto const& writeTargets = Operations.GetWriteTargets();
            for (auto const& targetId : writeTargets)
            {
                RenderTargets.push_back()
            }
        }
    };

    class RENDERCORE_API FrameGraph : public RefCountedObject
    {
    public:
        FrameGraph(class IRenderer* owner, int contextNum);
        ~FrameGraph();

        void Reset();
        void Compile();
        void Execute();

        // render
        const IRenderer* GetRenderer() const { return OwnerRenderer; }
        SceneView* GetSceneView(EViewType type) const { return Views[static_cast<int>(type)]; }
        TSet<PrimitiveSceneInfo*>& GetSceneInfos() { return SceneInfos; }
        void RegisterSceneInfo(PrimitiveSceneInfo* sceneInfo) { SceneInfos.insert(sceneInfo); }
        void UnregisterSceneInfo(PrimitiveSceneInfo* sceneInfo) { SceneInfos.erase(sceneInfo); }

        // Internal methods used by FrameGraphBuilder
        void AddPass(const String& name, PassOperations&& operations, PassExecutionFunction&& executeFunction, bool bIsMeshDrawPass = false);
        void SetPresentTarget(const FGRenderTarget& renderTarget);
        void RegisterRenderTarget(const FGRenderTarget& renderTarget);
        void ClearRenderTargetPool();

        // Command execution support
        FRenderContext* GetMainContext() const { return MainContext; }
        const TArray<FRenderContext*>& GetRenderContexts() const { return RenderContexts; }
        TArray<IRHICommand*>& GetCurrentAllCommands(int frontIndex) { return AllCommands[frontIndex]; }

        FORCEINLINE FrameGraphPass* GetPass(NameHandle name)
        {
            auto passIt = PassIndexMap.find(name);
            TAssertf(passIt->second < Passes.size(), "Invalid pass index found, pass name : \"%s\".", name.c_str());
            TAssertf(passIt != PassIndexMap.end(), "Pass not found, pass name : \"%s\".", name.c_str());
            return (passIt != PassIndexMap.end()) ? Passes[passIt->second].Get() : nullptr;
        }
        FORCEINLINE FrameGraphPass* GetPass(const String& name) { return GetPass(NameHandle{ name }); }
        FORCEINLINE FrameGraphPass* GetPass(int passIndex) { return passIndex < Passes.size() ? Passes[passIndex].Get() : nullptr; }

    private:
        // Initialize render contexts for multi-threading
        void InitializeRenderContexts(uint32 threadCount);

        void CullUnusedPasses() const;
        void TopologicalSort();
        void ScheduleRenderTargetLifetime();
        void AllocateRenderTargets();

        TArray<TRefCountPtr<FrameGraphPass>> Passes;
        TMap<NameHandle, int32> PassIndexMap;
        TArray<size_t> ExecutionOrder;
        THashMap<uint32, FGRenderTargetDesc> RenderTargetDescs;
        THashMap<uint32, RenderTextureRef> AllocatedRenderTargets;
        THashMap<uint32, std::pair<size_t, size_t>> RenderTargetLifetimes; // target -> (first_use, last_use)

        uint32 PresentTargetID = 0;
        bool bHasPresentTarget = false;

        RenderTargetPool Pool;

        // render
        IRenderer* OwnerRenderer { nullptr };
        TArray<SceneView*> Views;
        TSet<PrimitiveSceneInfo*> SceneInfos;

        // Command execution contexts
        FRenderContext* MainContext = nullptr;  // Main render thread context
        TArray<FRenderContext*> RenderContexts; // Worker thread contexts
        TArray<IRHICommand*> AllCommands[2];       // Consolidated commands for execution
    };

    #define EVENT_NAME(Name) Name
}
