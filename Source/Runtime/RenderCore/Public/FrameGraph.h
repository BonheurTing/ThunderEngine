#pragma once

#include "CoreMinimal.h"
//#include "IRenderer.h"
#include "RenderResource.h"
#include "RenderTargetPool.h"
#include "RenderContext.h"
#include "RenderPass.h"
#include "RHI.h"
#include "SceneView.h"

namespace Thunder
{
    class FRenderContext;
    class IRHICommand;

    class RENDERCORE_API PassOperations
    {
    public:
        void Read(const FGRenderTarget* renderTarget);
        void Read(const FGRenderTargetRef& renderTarget);
        void Write(const FGRenderTarget* renderTarget);
        void Write(const FGRenderTargetRef& renderTarget);

        const TArray<uint32>& GetReadTargets() const { return ReadTargets; }
        const TArray<uint32>& GetWriteTargets() const { return WriteTargets; }

    private:
        TArray<uint32> ReadTargets;
        TArray<uint32> WriteTargets;
    };

    using PassExecutionFunction = TFunction<void()>;

    struct FrameGraphPass : public RefCountedObject
    {
        FrameGraph* FrameGraph = nullptr;
        String Name;
        PassOperations Operations;
        PassExecutionFunction ExecuteFunction;
        bool bCulled = false;

        FrameGraphPass() = delete;
        FrameGraphPass(class FrameGraph* graph, const String& inName, PassOperations&& inOperations, PassExecutionFunction&& inExecuteFunction, bool inIsMeshDrawPass = false)
            : FrameGraph(graph), Name(inName), Operations(std::move(inOperations)), ExecuteFunction(std::move(inExecuteFunction)) {}
        PassOperations const& GetOperations() const { return Operations; }
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
        void SetPresentTarget(const FGRenderTarget* renderTarget);
        FORCEINLINE void SetPresentTarget(FGRenderTargetRef const& renderTarget) { SetPresentTarget(renderTarget.Get()); }
        void RegisterRenderTarget(FGRenderTarget* renderTarget);
        FORCEINLINE void RegisterRenderTarget(FGRenderTargetRef const& renderTarget) { RegisterRenderTarget(renderTarget.Get()); }
        void ClearRenderTargetPool();

        // Command execution support
        FRenderContext* GetMainContext() const { return MainContext; }
        const TArray<FRenderContext*>& GetRenderContexts() const { return RenderContexts; }
        TArray<IRHICommand*>& GetCurrentAllCommands(int frontIndex) { return AllCommands[frontIndex]; }

        FORCEINLINE void SetCurrentPass(FrameGraphPass* pass) const
        {
            MainContext->SetCurrentPass(pass);
            for (auto& context : RenderContexts)
            {
                context->SetCurrentPass(pass);
            }
        }
        FORCEINLINE FrameGraphPass* GetPass(NameHandle name)
        {
            auto passIt = PassIndexMap.find(name);
            TAssertf(passIt->second < Passes.size(), "Invalid pass index found, pass name : \"%s\".", name.c_str());
            TAssertf(passIt != PassIndexMap.end(), "Pass not found, pass name : \"%s\".", name.c_str());
            return (passIt != PassIndexMap.end()) ? Passes[passIt->second].Get() : nullptr;
        }
        FORCEINLINE FrameGraphPass* GetPass(const String& name) { return GetPass(NameHandle{ name }); }
        FORCEINLINE FrameGraphPass* GetPass(int passIndex) const { return passIndex < Passes.size() ? Passes[passIndex].Get() : nullptr; }
        bool GetRenderTargetFormat(uint32 renderTargetIndex, RHIFormat& outFormat, bool& outIsDepthStencil) const;

    private:
        // Initialize render contexts for multi-threading
        void InitializeRenderContexts(uint32 threadCount);

        void CullUnusedPasses() const;
        void TopologicalSort();
        void ScheduleRenderTargetLifetime();
        void AllocateRenderTargets();

        // Passes.
        TArray<TRefCountPtr<FrameGraphPass>> Passes;
        TMap<NameHandle, int32> PassIndexMap;
        TArray<size_t> ExecutionOrder;

        // Render targets.
        THashMap<uint32, FGRenderTargetRef> RenderTargets;
        THashMap<uint32, RenderTextureRef> AllocatedRenderTargets;
        THashMap<uint32, std::pair<size_t, size_t>> RenderTargetLifetimes; // target -> (first_use, last_use)
        uint32 PresentTargetID = 0;
        bool bHasPresentTarget = false;
        RenderTargetPool Pool;

        // Renderer.
        IRenderer* OwnerRenderer { nullptr };
        TArray<SceneView*> Views;
        TSet<PrimitiveSceneInfo*> SceneInfos;

        // Command execution contexts.
        FRenderContext* MainContext = nullptr;  // Main render thread context
        TArray<FRenderContext*> RenderContexts; // Worker thread contexts
        TArray<IRHICommand*> AllCommands[2];       // Consolidated commands for execution
    };

    #define EVENT_NAME(Name) Name
}
