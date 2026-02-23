#pragma once

#include "CoreMinimal.h"
#include "MeshDrawCommand.h"
#include "MeshPass.h"
#include "RenderTargetPool.h"
#include "RenderContext.h"
#include "RenderPass.h"
#include "RHI.h"
#include "RHICommand.h"
#include "SceneView.h"

namespace Thunder
{
    struct RenderContext;
    struct IRHICommand;
    class RHIUniformBuffer;
    struct ShaderParameterMap;

    struct TWriteTargetInfo
    {
        ELoadOp    LoadOp     = ELoadOp::Load;
        TVector4f  ClearColor = { 0.f, 0.f, 0.f, 1.f };
        float      ClearDepth   = 0.f;
        uint8      ClearStencil = 0;
    };

    class RENDERCORE_API PassOperations
    {
    public:
        void Read(const FGRenderTarget* renderTarget, NameHandle overrideRTName = NameHandle());
        void Write(const FGRenderTarget* renderTarget);
        // Write with explicit LoadOp::Clear and a custom clear color / depth-stencil value.
        void Write(const FGRenderTarget* renderTarget, const TVector4f& clearColor);
        void Write(const FGRenderTarget* renderTarget, float clearDepth, uint8 clearStencil);

        const TMap<uint32, NameHandle>& GetReadTargets() { return ReadTargets; }
        const TMap<uint32, TWriteTargetInfo>& GetWriteTargets() { return WriteTargets; }

        // Called by FrameGraph compile phase to mark a write target as first-use clear
        // using the clear value stored in the FGRenderTarget descriptor.
        void SetWriteTargetLoadOp(uint32 rtId, ELoadOp loadOp, const TVector4f& clearColor, float clearDepth = 1.f, uint8 clearStencil = 0);

    private:
        TMap<uint32, NameHandle> ReadTargets;
        TMap<uint32, TWriteTargetInfo> WriteTargets;
    };

    using PassExecutionFunction = TFunction<void()>;

    struct FrameGraphPass : public RefCountedObject
    {
        FrameGraph* FrameGraph = nullptr;
        String Name;
        PassOperations Operations;
        PassExecutionFunction ExecuteFunction;
        bool bCulled = false;
        // When true, this pass renders directly into the swapchain backbuffer.
        // No pool render target is written; the backbuffer is bound automatically.
        bool bIsPresentPass = false;

        // Shader bindings
        ShaderArchive* Archive = nullptr;
        TRefCountPtr<ShaderBindingsLayout> BindingLayout;
        TRefCountPtr<UniformBufferLayout> PassUBLayout;
        ShaderParameterMap* PassParameters = nullptr;
        TRefCountPtr<RHIUniformBuffer> PassUniformBuffer;
        bool bLayoutNeedsUpdate = true;

        FrameGraphPass() = delete;
        FrameGraphPass(class FrameGraph* graph, const String& inName, PassOperations&& inOperations, PassExecutionFunction&& inExecuteFunction)
            : FrameGraph(graph), Name(inName), Operations(std::move(inOperations)), ExecuteFunction(std::move(inExecuteFunction)) {}
        NameHandle GetName() { return Name; }
        PassOperations& GetOperations() { return Operations; }
        bool SetShader(NameHandle archiveName);
    };

    class RENDERCORE_API FrameGraph : public RefCountedObject
    {
    public:
        FrameGraph(class IRenderer* owner, int contextNum);
        ~FrameGraph();

        void Reset();
        void Compile();
        void Execute();

        // game
        void SetViewParameters(EViewType type, TVector4f cameraPos, const TMatrix44f& vpMatrix) const;

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
        FORCEINLINE void SetPresentPass(NameHandle passName) { PresentPassName = passName; }
        void RegisterRenderTarget(FGRenderTarget* renderTarget);
        FORCEINLINE void RegisterRenderTarget(FGRenderTargetRef const& renderTarget) { RegisterRenderTarget(renderTarget.Get()); }
        void ClearRenderTargetPool();

        // Command execution support
        RenderContext* GetMainContext() const { return MainContext; }
        const TArray<RenderContext*>& GetRenderContexts() const { return RenderContexts; }
        TArray<IRHICommand*>& GetCurrentAllCommands(int frontIndex) { return AllCommands[frontIndex]; }
        TArray<RHIPassState*>& GetCurrentPassStates(int frontIndex) { return AllPassStates[frontIndex]; }

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
        void AddRenderTargetClearOp();
        void SetPresentCommand();

        // Excute
        void AggregateContextCommands(uint32 frameIndex);
        void AddBeginFrameCommand(uint32 frameIndex);
        void AddBeginPassCommand(FrameGraphPass* pass, uint32 frameIndex);
        void AddPassState(uint32 frameIndex);
        void AddEndPassCommand(FrameGraphPass* pass, uint32 frameIndex);

        // Passes.
        TSet<NameHandle> CurrentFramePasses;
        NameHandle PresentPassName;
        TMap<NameHandle, TRefCountPtr<FrameGraphPass>> Passes;

        TArray<NameHandle> ExecutionOrder;

        // Render targets.
        THashMap<uint32, FGRenderTargetRef> RenderTargets;
        THashMap<uint32, TRefCountPtr<RenderTexture>> AllocatedRenderTargets;
        THashMap<uint32, std::pair<size_t, size_t>> RenderTargetLifetimes; // target -> (first_use, last_use)
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
        TArray<RHIPassState*> AllPassStates[2];

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
