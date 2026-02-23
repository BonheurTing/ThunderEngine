#pragma optimize("", off)
#include "FrameGraph.h"
#include <algorithm>
#include <UniformBuffer.h>
#include "RenderTexture.h"
#include "RenderContext.h"
#include "RHICommand.h"
#include "PlatformProcess.h"
#include "PrimitiveSceneInfo.h"
#include "RenderModule.h"
#include "ShaderArchive.h"
#include "ShaderModule.h"
#include "ShaderParameterMap.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskGraph.h"
#include "Concurrent/TaskScheduler.h"
#include "HAL/Event.h"
#include "Misc/CoreGlabal.h"


namespace Thunder
{
    class TaskDispatcher;

    void PassOperations::Read(const FGRenderTarget* renderTarget, NameHandle overrideRTName)
    {
        if (overrideRTName.IsEmpty())
        {
            ReadTargets.emplace(renderTarget->GetID(),renderTarget->GetName());
        }
        else
        {
            ReadTargets.emplace(renderTarget->GetID(), overrideRTName);
        }
    }

    void PassOperations::Write(const FGRenderTarget* renderTarget)
    {
        WriteTargets.emplace(renderTarget->GetID(), TWriteTargetInfo{});
    }

    void PassOperations::Write(const FGRenderTarget* renderTarget, const TVector4f& clearColor)
    {
        TWriteTargetInfo info;
        info.LoadOp = ELoadOp::Clear;
        info.ClearColor = clearColor;
        WriteTargets.emplace(renderTarget->GetID(), info);
    }

    void PassOperations::Write(const FGRenderTarget* renderTarget, float clearDepth, uint8 clearStencil)
    {
        TWriteTargetInfo info;
        info.LoadOp = ELoadOp::Clear;
        info.ClearDepth = clearDepth;
        info.ClearStencil = clearStencil;
        WriteTargets.emplace(renderTarget->GetID(), info);
    }

    void PassOperations::SetWriteTargetLoadOp(uint32 rtId, ELoadOp loadOp, const TVector4f& clearColor, float clearDepth, uint8 clearStencil)
    {
        auto it = WriteTargets.find(rtId);
        if (it == WriteTargets.end()) [[unlikely]]
        {
            TAssertf(false, "SetWriteTargetLoadOp: render target id %u is not in WriteTargets.", rtId);
            return;
        }
        it->second.LoadOp = loadOp;
        it->second.ClearColor = clearColor;
        it->second.ClearDepth = clearDepth;
        it->second.ClearStencil = clearStencil;
    }

    bool FrameGraphPass::SetShader(NameHandle archiveName)
    {
        Archive = ShaderModule::GetShaderArchive(archiveName);
        if (Archive == nullptr) [[unlikely]]
        {
            TAssertf(false, "Shader is not ready.");
            return false;
        }

        if (bLayoutNeedsUpdate)
        {
            BindingLayout = Archive->GetBindingsLayout();
            PassUBLayout = Archive->GetUniformBufferLayout(GetName());
            if (PassParameters == nullptr)
            {
                PassParameters = new ShaderParameterMap;
            }
            else
            {
                PassParameters->Reset();
            }
        }

        return true;
    }

    RHIPassState::RHIPassState(const RHIBeginPassCommand* beginCommand, uint32 commandId)
        : BeginCommandIndex(commandId)
    {
        bIsBackBufferPass = beginCommand->bIsBackBufferPass;
        RenderTargetCount = beginCommand->RenderTargetCount;
        for (uint32 i = 0; i < RenderTargetCount; ++i)
        {
            RenderTargets[i] = beginCommand->RenderTargets[i].Texture;
        }
        DepthStencil = beginCommand->DepthStencil.Texture;
    }

    FrameGraph::FrameGraph(IRenderer* owner, int contextNum) : OwnerRenderer(owner)
    {
        InitializeRenderContexts(contextNum);
        int viewNum = static_cast<int>(EViewType::Num);
        Views.resize(static_cast<size_t>(viewNum));
        for (int i = 0; i < viewNum; ++i)
        {
            Views[i] = new (TMemory::Malloc<SceneView>()) SceneView(this, static_cast<EViewType>(i));
        }
        CachedGlobalParameters = new ShaderParameterMap;
    }

    FrameGraph::~FrameGraph()
    {
        Reset();
        for (auto& view : Views)
        {
            TMemory::Destroy(view);
        }

        if (CachedGlobalParameters)
        {
            delete CachedGlobalParameters;
            CachedGlobalParameters = nullptr;
        }
        if (GlobalUniformBuffer)
        {
            delete GlobalUniformBuffer;
            GlobalUniformBuffer = nullptr;
        }

        for (auto& passParam : PassParameters | std::views::values)
        {
            if (passParam)
            {
                delete passParam;
                passParam = nullptr;
            }
        }
        PassParameters.clear();

        for (auto& ub : PassUniformBufferMap | std::views::values)
        {
            if (ub)
            {
                delete ub;
                ub = nullptr;
            }
        }
        PassUniformBufferMap.clear();
    }

    void FrameGraph::Reset()
    {
        // Release all allocated render targets back to pool
        for (const auto& [rtID, renderTarget] : AllocatedRenderTargets)
        {
            Pool.ReleaseRenderTarget(renderTarget);
        }

        for (auto it = Passes.begin(); it != Passes.end(); )
        {
            if (!CurrentFramePasses.contains(it->first) )
            {
                it = Passes.erase(it);
            }
            else
            {
                ++it;
            }
        }
        CurrentFramePasses.clear();
        PresentPassName = "";

        ExecutionOrder.clear();
        RenderTargets.clear();
        AllocatedRenderTargets.clear();
        RenderTargetLifetimes.clear();

        // Clear commands from previous frame
        uint32 frameIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
        AllCommands[frameIndex].clear();
        AllPassStates[frameIndex].clear();
        for (auto& context : RenderContexts)
        {
            context->ClearCommands();
            context->FreeAllocator();
        }
        MainContext->ClearCommands();
        MainContext->FreeAllocator();
    }

    void FrameGraph::Compile()
    {
        // Cull unused passes
        CullUnusedPasses();

        // Sort passes topologically
        TopologicalSort();

        // Calculate render target lifetimes
        ScheduleRenderTargetLifetime();

        // Allocate render targets with lifetime-based reuse
        AllocateRenderTargets();

        // Auto-set render targets LoadOp
        AddRenderTargetClearOp();

        // Add present command
        SetPresentCommand();
    }

    void FrameGraph::AggregateContextCommands(uint32 frameIndex)
    {
        // Add main context commands.
        const auto& mainCommands = MainContext->GetCommands();
        AllCommands[frameIndex].insert(AllCommands[frameIndex].end(), mainCommands.begin(), mainCommands.end());
        MainContext->ClearCommands();

        // Add threaded commands.
        for (auto& context : RenderContexts)
        {
            auto const& commands = context->GetCommands();
            if (!commands.empty())
            {
                AllCommands[frameIndex].insert(AllCommands[frameIndex].end(), commands.begin(), commands.end());
            }
            context->ClearCommands();
        }
    }

    void FrameGraph::AddBeginFrameCommand(uint32 frameIndex)
    {
        // Add begin frame command
        RHIBeginFrameCommand* beginFrameCommand = new (MainContext->Allocate<RHIBeginFrameCommand>()) RHIBeginFrameCommand;
        MainContext->AddCommand(beginFrameCommand);

        // Execute command before passes
        AggregateContextCommands(frameIndex);
    }

    void FrameGraph::AddBeginPassCommand(FrameGraphPass* curPass, uint32 frameIndex)
    {
        // Begin pass.
        RHIBeginPassCommand* newBeginCommand = new (MainContext->Allocate<RHIBeginPassCommand>()) RHIBeginPassCommand;

        // Handle read targets first (same for both regular and present passes).
        auto readRTs = curPass->GetOperations().GetReadTargets();
        for (const auto& rtId : readRTs | std::views::keys)
        {
            RenderTextureRef texture = GetAllocatedRenderTarget(rtId);
            if (!texture.IsValid())
            {
                TAssertf(false, "RenderTexture does not exist");
                continue;
            }
            if (texture->IsDepthStencilTargetable())
            {
                newBeginCommand->ReadDepthStencil.Texture   = texture->GetTextureRHI();
                newBeginCommand->ReadDepthStencil.OldState  = texture->GetTextureRHI()->GetState_RenderThread();
                newBeginCommand->ReadDepthStencil.bNeedBarrier = newBeginCommand->ReadDepthStencil.OldState != ERHIResourceState::DepthRead;
                if (newBeginCommand->ReadDepthStencil.bNeedBarrier)
                {
                    texture->GetTextureRHI()->SetState_RenderThread(ERHIResourceState::DepthRead);
                }
            }
            else if (texture->IsRenderTargetable())
            {
                ERHIResourceState oldState  = texture->GetTextureRHI()->GetState_RenderThread();
                bool bNeedBarrier           = oldState != ERHIResourceState::AllShaderResource;
                newBeginCommand->ReadRenderTargets.push_back(TReadTextureBinding{ .Texture = texture->GetTextureRHI(), .bNeedBarrier = bNeedBarrier, .OldState = oldState });
                if (bNeedBarrier)
                {
                    texture->GetTextureRHI()->SetState_RenderThread(ERHIResourceState::AllShaderResource);
                }
            }
            else
            {
                TAssertf(false, "RenderTexture does not have valid view");
            }
        }

        if (curPass->bIsPresentPass)
        {
            // Present pass writes directly to the swapchain backbuffer.
            // The actual barrier and RT binding are handled at execute time via bIsBackBufferPass.
            newBeginCommand->bIsBackBufferPass   = true;
            newBeginCommand->RenderTargetCount   = 0;
            AllCommands[frameIndex].push_back(newBeginCommand);
            return;
        }

        // Regular pass: bind pool render targets.
        auto& writeRTs = curPass->GetOperations().GetWriteTargets();
        uint32 rtIndex = 0;
        for (const auto& [rtId, writeInfo] : writeRTs)
        {
            RenderTextureRef texture = GetAllocatedRenderTarget(rtId);
            if (!texture.IsValid())
            {
                TAssertf(false, "RenderTexture does not exist");
                continue;
            }
            if (texture->IsDepthStencilTargetable())
            {
                newBeginCommand->DepthStencil.Texture      = texture->GetTextureRHI();
                newBeginCommand->DepthStencil.LoadOp       = writeInfo.LoadOp;
                newBeginCommand->DepthStencil.ClearDepth   = writeInfo.ClearDepth;
                newBeginCommand->DepthStencil.ClearStencil = writeInfo.ClearStencil;
                newBeginCommand->DepthStencil.OldState     = texture->GetTextureRHI()->GetState_RenderThread();
                newBeginCommand->DepthStencil.bNeedBarrier = newBeginCommand->DepthStencil.OldState != ERHIResourceState::DepthWrite;
                if (newBeginCommand->DepthStencil.bNeedBarrier)
                {
                    texture->GetTextureRHI()->SetState_RenderThread(ERHIResourceState::DepthWrite);
                }
            }
            else if (texture->IsRenderTargetable())
            {
                auto& binding        = newBeginCommand->RenderTargets[rtIndex++];
                binding.Texture      = texture->GetTextureRHI();
                binding.LoadOp       = writeInfo.LoadOp;
                binding.ClearColor   = writeInfo.ClearColor;
                binding.OldState     = texture->GetTextureRHI()->GetState_RenderThread();
                binding.bNeedBarrier = binding.OldState != ERHIResourceState::RenderTarget;
                if (binding.bNeedBarrier)
                {
                    texture->GetTextureRHI()->SetState_RenderThread(ERHIResourceState::RenderTarget);
                }
            }
            else
            {
                TAssertf(false, "RenderTexture does not have valid view");
            }
        }
        newBeginCommand->RenderTargetCount = rtIndex;

        AllCommands[frameIndex].push_back(newBeginCommand);
    }

    void FrameGraph::AddPassState(uint32 frameIndex)
    {
        uint32 commandId = static_cast<uint32>(AllCommands[frameIndex].size()) - 1;
        auto const lastCommand = AllCommands[frameIndex][commandId];
        if (auto const beginCommand = dynamic_cast<RHIBeginPassCommand*>(lastCommand)) // todo dynamic_cast
        {
            AllPassStates[frameIndex].push_back(new RHIPassState(beginCommand, commandId));
        }
    }

    void FrameGraph::AddEndPassCommand(FrameGraphPass* curPass, uint32 frameIndex)
    {
        // End pass.
        RHIEndPassCommand* newEndCommand = new (MainContext->Allocate<RHIEndPassCommand>()) RHIEndPassCommand;

        if (curPass->bIsPresentPass)
        {
            // Present pass: transition the backbuffer from RenderTarget -> Present at execute time.
            newEndCommand->bIsBackBufferPass = true;
        }

        AllCommands[frameIndex].push_back(newEndCommand);
    }

    void FrameGraph::Execute()
    {
        uint32 frameIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
        AggregateContextCommands(frameIndex);

        AllPassStates[frameIndex].reserve(ExecutionOrder.size());
        // Execute passes in order.
        for (size_t i = 0; i < ExecutionOrder.size(); ++i)
        {
            NameHandle passName = ExecutionOrder[i];
            if (!Passes.contains(passName) || Passes[passName]->bCulled) [[unlikely]]
            {
                TAssertf(false, "Execution invalid pass \"%s\".", passName);
                continue;
            }

            {
                auto& pass = Passes[passName];
                // Execute pass.
                SetCurrentPass(pass.Get());
                pass->ExecuteFunction();

                AddBeginPassCommand(pass, frameIndex);
                AddPassState(frameIndex);
                AggregateContextCommands(frameIndex);
                AddEndPassCommand(pass, frameIndex);
            }

            // After pass execution, release render targets that are no longer needed.
            for (const auto& [rtID, lifetime] : RenderTargetLifetimes)
            {
                if (lifetime.second != i) continue; // Not the last use yet.

                auto it = AllocatedRenderTargets.find(rtID);
                if (it != AllocatedRenderTargets.end())
                {
                    Pool.ReleaseRenderTarget(it->second);
                }
            }
        }
    }

    void FrameGraph::SetViewParameters(EViewType type, TVector4f cameraPos, const TMatrix44f& vpMatrix) const
    {
        // Render thread.
        auto globalParameters = GetGlobalParameters();
        globalParameters->SetVectorParameter("CameraPosition", cameraPos);
        globalParameters->SetVectorParameter("ViewProjectionMatrix0", vpMatrix.GetColumn(0));
        globalParameters->SetVectorParameter("ViewProjectionMatrix1", vpMatrix.GetColumn(1));
        globalParameters->SetVectorParameter("ViewProjectionMatrix2", vpMatrix.GetColumn(2));
        globalParameters->SetVectorParameter("ViewProjectionMatrix3", vpMatrix.GetColumn(3));
        TMatrix44f invVPMatrix = vpMatrix.Inverse();
        globalParameters->SetVectorParameter("InvViewProjectionMatrix0", invVPMatrix.GetColumn(0));
        globalParameters->SetVectorParameter("InvViewProjectionMatrix1", invVPMatrix.GetColumn(1));
        globalParameters->SetVectorParameter("InvViewProjectionMatrix2", invVPMatrix.GetColumn(2));
        globalParameters->SetVectorParameter("InvViewProjectionMatrix3", invVPMatrix.GetColumn(3));
    }

    // Called on game thread.
    void FrameGraph::RegisterSceneInfo_GameThread(PrimitiveSceneInfo* sceneInfo)
    {
        uint32 const gameThreadIndex = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) % 2;
        SceneInfoRegistrationSet[gameThreadIndex].insert(sceneInfo);
        SceneInfoUpdateSet[gameThreadIndex].insert(sceneInfo);
    }

    void FrameGraph::UnregisterSceneInfo_GameThread(PrimitiveSceneInfo* sceneInfo)
    {
        uint32 const gameThreadIndex = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) % 2;
        SceneInfoUnregistrationSet[gameThreadIndex].insert(sceneInfo);
        SceneInfoUpdateSet[gameThreadIndex].erase(sceneInfo);
    }

    void FrameGraph::UpdateSceneInfo_GameThread(PrimitiveSceneInfo* sceneInfo)
    {
        uint32 const gameThreadIndex = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) % 2;
        SceneInfoUpdateSet[gameThreadIndex].insert(sceneInfo);
    }

    void FrameGraph::UpdateSceneInfo_RenderThread()
    {
        uint32 const renderThreadIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;

        // Register.
        auto const& registerRequests = SceneInfoRegistrationSet[renderThreadIndex];
        for (auto const& registerRequest : registerRequests)
        {
            //registerRequest->CreateUniformBuffer();
            SceneInfos.insert(registerRequest);
        }

        // Unregister.
        auto const& unregisterRequests = SceneInfoUnregistrationSet[renderThreadIndex];
        for (auto const& unregisterRequest : unregisterRequests)
        {
            SceneInfos.erase(unregisterRequest);
        }

        // Update.
        SceneInfoCurrentUpdateSet.clear();
        SceneInfoCurrentUpdateSet.swap(SceneInfoUpdateSet[renderThreadIndex]);
    }

    void FrameGraph::UpdatePassSceneInfo(EMeshPass passType)
    {
        // Get scene infos to update.
        TArray<PrimitiveSceneInfo*> sceneInfos{};
        for (auto const& sceneInfo : SceneInfoCurrentUpdateSet)
        {
            sceneInfos.push_back(sceneInfo);
        }
        uint32 const sceneInfoCount = static_cast<uint32>(sceneInfos.size());
        if (sceneInfoCount == 0)
        {
            return;
        }

        // Dispatch update tasks.
        const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
        auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
        dispatcher->Promise(static_cast<int>(sceneInfoCount));
        uint32 numThread = static_cast<uint32>(GetRenderContexts().size());
        GSyncWorkers->ParallelFor([this, &sceneInfos, dispatcher, sceneInfoCount, passType](uint32 bundleBegin, uint32 bundleSize, uint32 threadId)
        {
            auto const& contexts = GetRenderContexts();
            auto context = contexts[threadId];
            for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
            {
                if (index >= sceneInfoCount)
                {
                    break;
                }

                // Cache static mesh-draw commands for current pass.
                for (auto& sceneInfo : sceneInfos)
                {
                    if (sceneInfo->IsMeshDrawCacheSupported())
                    {
                        sceneInfo->CacheMeshDrawCommand(context, passType);
                    }
                }

                dispatcher->Notify();
            }
        }, sceneInfoCount, numThread);

        // Wait for task to finish.
        doWorkEvent->Wait();
        FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
        TMemory::Destroy(dispatcher);

        // todo uptda

        // Finalize commands.
        for (auto& context : RenderContexts)
        {
            auto const& cachedCommands = context->GetCachedCommands();
            for (auto& cachedCommandEntry : cachedCommands)
            {
                // Get mesh batch and command.
                auto const& meshBatch = std::get<0>(cachedCommandEntry);
                auto const& command = std::get<1>(cachedCommandEntry);
                PrimitiveSceneInfo* sceneInfo = meshBatch->GetSceneInfo();

                // Cache mesh-draw command.
                auto cachedDrawListIt = CachedDrawLists.find(passType);
                if (cachedDrawListIt == CachedDrawLists.end())
                {
                    CachedDrawLists[passType] = CachedPassMeshDrawList{};
                }
                CachedPassMeshDrawList& cachedDrawList = CachedDrawLists[passType];
                uint64 commandIndex = command->CachedCommandIndex;
                cachedDrawList.MeshDrawCommands[commandIndex] = command;

                // Save mesh draw info.
                sceneInfo->EmplaceDrawCommandInfo(passType, meshBatch->GetKey(), commandIndex);
            }

            context->ClearCachedCommands();
        }
    }

    void FrameGraph::ResolveVisibility(EViewType viewType, EMeshPass passType)
    {
        // Update pass.
        UpdatePassSceneInfo(passType);

        // Cull.
        auto view = GetSceneView(viewType);
        if (!view->IsCulled())
        {
            view->CullSceneProxies();
        }

        // Save visible cached mesh-draw commands.
        auto const& visibleSceneInfos = view->GetVisibleStaticSceneInfos();
        auto& visibleCachedDrawList = VisibleCachedDrawLists[passType];
        visibleCachedDrawList.clear();
        for (auto const& sceneInfo : visibleSceneInfos)
        {
            bool const isStatic = sceneInfo->IsMeshDrawCacheSupported();
            if (!isStatic) [[unlikely]]
            {
                TAssertf(false, "Trying to cache a dynamic mesh batch.");
                continue;
            }

            // Add cached command.
            auto const& staticMeshes = sceneInfo->GetStaticMeshes();
            for (const auto& batchKey : staticMeshes | std::views::keys)
            {
                auto const& commandInfo = sceneInfo->GetDrawCommandInfo(passType, batchKey);
                auto commandIt = CachedDrawLists[passType].MeshDrawCommands.find(commandInfo.CommandIndex);
                if (commandIt == CachedDrawLists[passType].MeshDrawCommands.end())
                {
                    TAssertf(false, "Mesh draw command index is invalid, this mesh draw is not cached yet.");
                    continue;
                }
                visibleCachedDrawList.push_back(commandIt->second);
            }
        }
    }

    void FrameGraph::AddPass(const String& name, PassOperations&& operations, PassExecutionFunction&& executeFunction)
    {
        auto pass = Passes.find(name);
        if (pass == Passes.end())
        {
            auto newPass = MakeRefCount<FrameGraphPass>(this, name, std::move(operations), std::move(executeFunction));
            Passes[name] = newPass;
        }
        else
        {
            pass->second->Operations = std::move(operations);
            pass->second->ExecuteFunction = std::move(executeFunction);
        }
        CurrentFramePasses.emplace(name);
    }

    bool FrameGraph::GetRenderTargetFormat(uint32 renderTargetIndex, RHIFormat& outFormat, bool& outIsDepthStencil) const
    {
        auto it = RenderTargets.find(renderTargetIndex);
        if (it == RenderTargets.end())
        {
            TAssertf(false, "Render target not found, index : %d.", renderTargetIndex);
            return false;
        }
        auto const& desc = it->second->GetDesc();
        outFormat = desc.Format;
        outIsDepthStencil = desc.bIsDepthStencil;
        return true;
    }

    RenderTextureRef FrameGraph::GetAllocatedRenderTarget(uint32 textureID)
    {
        auto it = AllocatedRenderTargets.find(textureID);
        if (it != AllocatedRenderTargets.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void FrameGraph::InitGlobalUniformBuffer()
    {
        const auto layout = ShaderModule::GetGlobalUniformBufferLayout();
        if (!layout) [[unlikely]]
        {
            TAssertf(false, "Cannot update uniform buffer: UniformBufferLayout \"global\" not found.");
            return;
        }

        byte* constantData = RenderModule::SetupUniformBufferParameters(MainContext, layout, CachedGlobalParameters, "Global");
        if (GlobalUniformBuffer.IsValid())
        {
            //RHIDeferredDeleteResource(std::move(GlobalUniformBuffer));
            RHIUpdateUniformBuffer(MainContext, GlobalUniformBuffer, constantData);
        }
        else
        {
            GlobalUniformBuffer = RHICreateUniformBuffer(layout->GetTotalSize(), EUniformBufferFlags::UniformBuffer_SingleFrame, constantData);
        }
    }

    ShaderParameterMap* FrameGraph::GetPassParameters(EMeshPass pass)
    {
        auto it = PassParameters.find(pass);
        if (it == PassParameters.end())
        {
            ShaderParameterMap* defaultParam = ShaderModule::GetPassDefaultParameters(pass);
            auto [newIt, success] = PassParameters.emplace(pass, defaultParam);
            TAssertf(success, "Pass parameter already exists." );
            PassUniformBufferMap.emplace(pass, nullptr);
            return newIt->second;
        }
        return it->second;
    }

    void FrameGraph::UpdatePassParameters(EMeshPass pass, const ShaderParameterMap* parameters, const String& ubName)
    {
        const auto layout = ShaderModule::GetPassUniformBufferLayout(pass);
        if (!layout) [[unlikely]]
        {
            TAssertf(false, "Cannot update uniform buffer: UniformBufferLayout \"%s\" not found.", ubName);
            return;
        }
        TRefCountPtr<RHIUniformBuffer>& passUniformBuffer = PassUniformBufferMap.at(pass);
        byte* constantData = RenderModule::SetupUniformBufferParameters(MainContext, layout, parameters, ubName);
        if (passUniformBuffer.IsValid())
        {
            //RHIDeferredDeleteResource(std::move(passUniformBuffer));
            RHIUpdateUniformBuffer(MainContext, passUniformBuffer, constantData);
        }
        else
        {
            passUniformBuffer = RHICreateUniformBuffer(layout->GetTotalSize(), EUniformBufferFlags::UniformBuffer_SingleFrame, constantData);
        }
    }

    const RHIUniformBuffer* FrameGraph::GetPassUniformBuffer(EMeshPass pass) const
    {
        auto it = PassUniformBufferMap.find(pass);
        if (it == PassUniformBufferMap.end()) [[unlikely]]
        {
            TAssertf(false, "Pass \"%s\" unform buffer is not exists.", pass);
            return nullptr;
        }
        return it->second;
    }

    void FrameGraph::InitializeRenderContexts(uint32 threadCount)
    {
        if (!MainContext)
        {
            MainContext = new RenderContext(this);
        }

        // Clear existing contexts
        for (auto& context : RenderContexts)
        {
            delete context;
        }
        RenderContexts.clear();

        // Create new contexts for each thread
        RenderContexts.reserve(threadCount);
        for (uint32 i = 0; i < threadCount; ++i)
        {
            RenderContexts.push_back(new RenderContext(this));
        }
    }

    void FrameGraph::RegisterRenderTarget(FGRenderTarget* renderTarget)
    {
        uint32 id = static_cast<uint32>(RenderTargets.size());
        RenderTargets[id] = renderTarget;
        RenderTargets[id]->SetId(id);
    }

    void FrameGraph::ClearRenderTargetPool()
    {
        Pool.TickUnusedFrameCounters();
        Pool.ReleaseLongUnusedTargets();
    }

    void FrameGraph::CullUnusedPasses() const
    {
        // Mark all current passes as culled initially.
        for (const auto& pass : Passes | std::views::values)
        {
            pass->bCulled = true;
        }

        TQueue<uint32> targetQueue;
        THashSet<uint32> neededTargets;

        if (PresentPassName.IsEmpty()) [[unlikely]]
        {
            TAssertf(false, "Present pass is not set. Call SetPresentPass() before Compile().");
            return;
        }

        auto presentPassIt = Passes.find(PresentPassName);
        if (presentPassIt == Passes.end()) [[unlikely]]
        {
            TAssertf(false, "Present pass \"%s\" not found in frame graph.", PresentPassName.c_str());
            return;
        }

        // The present pass itself is always needed.
        presentPassIt->second->bCulled = false;

        for (uint32 readTarget : presentPassIt->second->Operations.GetReadTargets() | std::views::keys)
        {
            if (!neededTargets.contains(readTarget))
            {
                neededTargets.insert(readTarget);
                targetQueue.push(readTarget);
            }
        }

        // Backwards traversal to find passes that produce the needed render targets.
        while (!targetQueue.empty())
        {
            uint32 currentTarget = targetQueue.front();
            targetQueue.pop();

            // Find passes that write to this target.
            for (auto& pass : Passes)
            {
                if (!CurrentFramePasses.contains(pass.first))
                {
                    continue;
                }
                const auto& writeTargets = pass.second->Operations.GetWriteTargets();
                if (writeTargets.contains(currentTarget))
                {
                    pass.second->bCulled = false;

                    // Add read targets of this pass to the queue.
                    for (uint32 readTarget : pass.second->Operations.GetReadTargets() | std::views::keys)
                    {
                        if (!neededTargets.contains(readTarget))
                        {
                            neededTargets.insert(readTarget);
                            targetQueue.push(readTarget);
                        }
                    }
                }
            }
        }
    }

    void FrameGraph::TopologicalSort()
    {
        ExecutionOrder.clear();

        TMap<NameHandle, uint32> inDegreeMap;

        // Calculate in-degrees
        for (auto& passX : Passes)
        {
            if (passX.second->bCulled) continue;
            inDegreeMap.emplace(passX.first, 0);

            for (uint32 readTarget : passX.second->Operations.GetReadTargets() | std::views::keys)
            {
                for (auto& passY : Passes)
                {
                    if (passX.first == passY.first || passY.second->bCulled) continue;

                    const auto& writeTargets = passY.second->Operations.GetWriteTargets();
                    if (writeTargets.contains(readTarget))
                    {
                        inDegreeMap[passX.first]++;
                    }
                }
            }
        }
    

        // Kahn's algorithm
        TQueue<NameHandle> queue;
        for (auto& pass : Passes)
        {
            if (!pass.second->bCulled)
            {
                auto const& inDegree = inDegreeMap.find(pass.first);
                if (inDegree != inDegreeMap.end() && inDegree->second == 0)
                {
                    queue.push(pass.first);
                }
            }
        }

        while (!queue.empty())
        {
            NameHandle current = queue.front();
            queue.pop();
            ExecutionOrder.push_back(current);

            // Update in-degrees for dependent passes
            for (uint32 writeTarget : Passes[current]->Operations.GetWriteTargets() | std::views::keys)
            {
                for (auto& pass : Passes)
                {
                    if (pass.second->bCulled) continue;
                    const auto& readTargets = pass.second->Operations.GetReadTargets();
                    if (readTargets.contains(writeTarget))
                    {
                        inDegreeMap[pass.first]--;
                        if (inDegreeMap[pass.first] == 0)
                        {
                            queue.push(pass.first);
                        }
                    }
                }
            }
        }
    }

    void FrameGraph::ScheduleRenderTargetLifetime()
    {
        RenderTargetLifetimes.clear();

        // Calculate lifetimes based on execution order
        for (size_t execIndex = 0; execIndex < ExecutionOrder.size(); ++execIndex)
        {
            NameHandle passName = ExecutionOrder[execIndex];
            const auto& pass = Passes[passName];

            // Update lifetimes for read targets
            for (uint32 rtID : pass->Operations.GetReadTargets() | std::views::keys)
            {
                if (!RenderTargetLifetimes.contains(rtID))
                {
                    RenderTargetLifetimes[rtID] = {execIndex, execIndex};
                }
                else
                {
                    RenderTargetLifetimes[rtID].second = execIndex;
                }
            }

            // Update lifetimes for write targets
            for (uint32 rtID : pass->Operations.GetWriteTargets() | std::views::keys)
            {
                if (!RenderTargetLifetimes.contains(rtID))
                {
                    RenderTargetLifetimes[rtID] = {execIndex, execIndex};
                }
                else
                {
                    RenderTargetLifetimes[rtID].first = std::min(RenderTargetLifetimes[rtID].first, execIndex);
                    RenderTargetLifetimes[rtID].second = execIndex;
                }
            }
        }
    }

    void FrameGraph::AllocateRenderTargets()
    {
        // Group render targets by their allocation time and compatible specs for reuse
        TArray<std::pair<uint32, std::pair<size_t, size_t>>> sortedLifetimes;
        for (const auto& [rtID, lifetime] : RenderTargetLifetimes)
        {
            sortedLifetimes.emplace_back(rtID, lifetime);
        }

        // Sort by allocation time (first_use)
        std::sort(sortedLifetimes.begin(), sortedLifetimes.end(),
            [](const auto& a, const auto& b) {
                return a.second.first < b.second.first;
            });

        // Track when render targets become available for reuse
        TArray<std::pair<size_t, uint32>> availableTargets; // (end_time, rtID)

        for (const auto& [rtID, lifetime] : sortedLifetimes)
        {
            if (RenderTargets.find(rtID) == RenderTargets.end())
            {
                continue;
            }

            const auto& renderTarget = RenderTargets[rtID];
            RenderTextureRef allocatedTarget = nullptr;

            // Check if we can reuse a render target that has finished its lifetime
            for (auto it = availableTargets.begin(); it != availableTargets.end(); ++it)
            {
                if (it->first < lifetime.first) // Target is available before we need it
                {
                    uint32 availableRtID = it->second;
                    if (AllocatedRenderTargets.find(availableRtID) != AllocatedRenderTargets.end())
                    {
                        auto availableTarget = AllocatedRenderTargets[availableRtID];
                        const auto& availableFrameGraphTarget = RenderTargets[availableRtID];

                        // Check if specs are compatible for reuse
                        if (availableFrameGraphTarget->IsSame(renderTarget.Get()))
                        {
                            allocatedTarget = availableTarget;
                            availableTargets.erase(it);
                            break;
                        }
                    }
                }
            }

            // If no reusable target found, allocate new one from pool
            if (!allocatedTarget)
            {
                allocatedTarget = Pool.AcquireRenderTarget(renderTarget->GetDesc());
            }

            AllocatedRenderTargets[rtID] = allocatedTarget;

            // Schedule this target to be available after its lifetime ends
            availableTargets.emplace_back(lifetime.second, rtID);

            // Keep availableTargets sorted by end time
            std::sort(availableTargets.begin(), availableTargets.end());
        }
    }

    void FrameGraph::AddRenderTargetClearOp()
    {
        // Auto-set LoadOp::Clear for render targets that are written for the first time
        // and have a clear value defined in their descriptor. Explicit WriteClear() calls
        // are already recorded in the pass operations, so only override targets that are
        // still using the default ELoadOp::Load.
        for (size_t execIndex = 0; execIndex < ExecutionOrder.size(); ++execIndex)
        {
            NameHandle passName = ExecutionOrder[execIndex];
            auto& pass = Passes[passName];
            if (pass->bCulled) continue;

            for (auto& [rtId, writeInfo] : pass->Operations.GetWriteTargets())
            {
                // Only promote to Clear if the caller hasn't already set an explicit LoadOp.
                if (writeInfo.LoadOp != ELoadOp::Load) continue;

                // Check this is the first use (first_use == current execIndex).
                auto lifetimeIt = RenderTargetLifetimes.find(rtId);
                if (lifetimeIt == RenderTargetLifetimes.end()) continue;
                if (lifetimeIt->second.first != execIndex) continue;

                // Only auto-clear when the FGRenderTarget descriptor carries a clear value.
                auto rtIt = RenderTargets.find(rtId);
                if (rtIt == RenderTargets.end()) continue;
                const FGRenderTargetDesc& desc = rtIt->second->GetDesc();
                if (!desc.bHasClearValue) continue;

                pass->Operations.SetWriteTargetLoadOp(rtId, ELoadOp::Clear,
                    desc.ClearValue, desc.ClearDepth, desc.ClearStencil);
            }
        }
    }

    void FrameGraph::SetPresentCommand()
    {
        auto passIt = Passes.find(PresentPassName);
        if (passIt == Passes.end()) [[unlikely]]
        {
            TAssertf(false, "Present pass \"%s\" not found.", PresentPassName.c_str());
            return;
        }
        passIt->second->bIsPresentPass = true;
    }
}
