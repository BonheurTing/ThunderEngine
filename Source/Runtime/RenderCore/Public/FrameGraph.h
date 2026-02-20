#pragma once

#include "CoreMinimal.h"
#include "MeshDrawCommand.h"
#include "MeshPass.h"
#include "RenderTargetPool.h"
#include "RenderContext.h"
#include "RenderPass.h"
#include "RHI.h"
#include "SceneView.h"

namespace Thunder
{
    struct RenderContext;
    struct IRHICommand;
    class RHIUniformBuffer;
    struct ShaderParameterMap;

    class RENDERCORE_API PassOperations
    {
    public:
        void Read(const FGRenderTarget* renderTarget, NameHandle rtName = NameHandle());
        void Read(FGRenderTargetRef const& renderTarget, NameHandle rtName = NameHandle());
        void Write(const FGRenderTarget* renderTarget, NameHandle rtName = NameHandle());
        void Write(FGRenderTargetRef const& renderTarget, NameHandle rtName = NameHandle());

        const TMap<uint32, NameHandle>& GetReadTargets() { return ReadTargets; }
        const TMap<uint32, NameHandle>& GetWriteTargets() { return WriteTargets; }

    private:
        TMap<uint32, NameHandle> ReadTargets;
        TMap<uint32, NameHandle> WriteTargets;
    };

    using PassExecutionFunction = TFunction<void()>;

    struct FrameGraphPass : public RefCountedObject
    {
        FrameGraph* FrameGraph = nullptr;
        String Name;
        PassOperations Operations;
        PassExecutionFunction ExecuteFunction;
        bool bCulled = false;

        // Shader bindings
        TRefCountPtr<ShaderBindingsLayout> BindingLayout;
        TRefCountPtr<UniformBufferLayout> PassUBLayout;
        ShaderParameterMap* PassParameters = nullptr;
        TRefCountPtr<RHIUniformBuffer> PassUniformBuffer;
        bool bLayoutNeedsUpdate = true;

        FrameGraphPass() = delete;
        FrameGraphPass(class FrameGraph* graph, const String& inName, PassOperations&& inOperations, PassExecutionFunction&& inExecuteFunction)
            : FrameGraph(graph), Name(inName), Operations(std::move(inOperations)), ExecuteFunction(std::move(inExecuteFunction)) {}
        PassOperations& GetOperations() { return Operations; }
        RENDERCORE_API TMap<NameHandle, uint32> GetFGTextures();
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
        void RegisterSceneInfo_GameThread(PrimitiveSceneInfo* sceneInfo);
        void UnregisterSceneInfo_GameThread(PrimitiveSceneInfo* sceneInfo);
        void UpdateSceneInfo_GameThread(PrimitiveSceneInfo* sceneInfo);
        void UpdateSceneInfo_RenderThread();
        void UpdatePassSceneInfo(EMeshPass passType);
        void ResolveVisibility(EViewType viewType, EMeshPass passType);

        // Internal methods used by FrameGraphBuilder
        void AddPass(const String& name, PassOperations&& operations, PassExecutionFunction&& executeFunction);
        void SetPresentTarget(const FGRenderTarget* renderTarget);
        FORCEINLINE void SetPresentTarget(FGRenderTargetRef const& renderTarget) { SetPresentTarget(renderTarget.Get()); }
        void RegisterRenderTarget(FGRenderTarget* renderTarget);
        FORCEINLINE void RegisterRenderTarget(FGRenderTargetRef const& renderTarget) { RegisterRenderTarget(renderTarget.Get()); }
        void ClearRenderTargetPool();

        // Command execution support
        RenderContext* GetMainContext() const { return MainContext; }
        const TArray<RenderContext*>& GetRenderContexts() const { return RenderContexts; }
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
            auto passIt = Passes.find(name);
            TAssertf(passIt != Passes.end(), "Pass not found, pass name : \"%s\".", name.c_str());
            return (passIt != Passes.end()) ? passIt->second.Get() : nullptr;
        }
        bool GetRenderTargetFormat(uint32 renderTargetIndex, RHIFormat& outFormat, bool& outIsDepthStencil) const;
        TRefCountPtr<RenderTexture> GetAllocatedRenderTarget(uint32 textureID);

        TArray<RHICachedDrawCommand*> const& GetVisibleCachedDrawList(EMeshPass passType) { return VisibleCachedDrawLists[passType]; }

        ShaderParameterMap* GetGlobalParameters() const { return CachedGlobalParameters; }
        void InitGlobalUniformBuffer();
        ShaderParameterMap* GetPassParameters(EMeshPass pass);
        void UpdatePassParameters(EMeshPass pass, const ShaderParameterMap* parameters, const String& ubName);
        const RHIUniformBuffer* GetGlobalUniformBuffer() const { return GlobalUniformBuffer.IsValid() ? GlobalUniformBuffer.Get() : nullptr; }
        const RHIUniformBuffer* GetPassUniformBuffer(EMeshPass pass) const;
         
    private:
        // Initialize render contexts for multi-threading
        void InitializeRenderContexts(uint32 threadCount);

        void CullUnusedPasses() const;
        void TopologicalSort();
        void ScheduleRenderTargetLifetime();
        void AllocateRenderTargets();

        // Excute
        void IntegrateCommands(uint32 frameIndex);
        void FlushCommands(uint32 frameIndex);

        // Passes.
        
        TSet<NameHandle> CurrentFramePasses;
        TMap<NameHandle, TRefCountPtr<FrameGraphPass>> Passes;

        TArray<NameHandle> ExecutionOrder;

        // Render targets.
        THashMap<uint32, FGRenderTargetRef> RenderTargets;
        THashMap<uint32, TRefCountPtr<RenderTexture>> AllocatedRenderTargets;
        THashMap<uint32, std::pair<size_t, size_t>> RenderTargetLifetimes; // target -> (first_use, last_use)
        uint32 PresentTargetID = 0;
        bool bHasPresentTarget = false;
        RenderTargetPool Pool;

        // Renderer.
        IRenderer* OwnerRenderer { nullptr };
        TArray<SceneView*> Views;
        TSet<PrimitiveSceneInfo*> SceneInfos;
        TSet<PrimitiveSceneInfo*> SceneInfoUpdateSet[2]; // Game thread and render thread double buffer.
        TSet<PrimitiveSceneInfo*> SceneInfoRegistrationSet[2];
        TSet<PrimitiveSceneInfo*> SceneInfoUnregistrationSet[2];
        TSet<PrimitiveSceneInfo*> SceneInfoCurrentUpdateSet; // Update CacheMeshDrawCommand

        TSet<PrimitiveSceneInfo*> PrimitivesNeedingUniformBufferUpdate; // Update Primitive UniformBuffer

        // Command execution contexts.
        RenderContext* MainContext = nullptr;  // Main render thread context
        TArray<RenderContext*> RenderContexts; // Worker thread contexts
        TArray<IRHICommand*> AllCommands[2];       // Consolidated commands for execution

        // Mesh-draw.
        TMap<EMeshPass, CachedPassMeshDrawList> CachedDrawLists;
        TMap<EMeshPass, TArray<RHICachedDrawCommand*>> VisibleCachedDrawLists;

        // Uniform buffer.
        ShaderParameterMap* CachedGlobalParameters = nullptr;
        TRefCountPtr<RHIUniformBuffer> GlobalUniformBuffer;
        TMap<EMeshPass, ShaderParameterMap*> PassParameters;
        TMap<EMeshPass, TRefCountPtr<RHIUniformBuffer>> PassUniformBufferMap;
    };

    #define EVENT_NAME(Name) Name
}
