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

    void PassOperations::Read(const FGRenderTarget* renderTarget)
    {
        ReadTargets.push_back(renderTarget->GetID());
    }

    void PassOperations::Read(const FGRenderTargetRef& renderTarget)
    {
        ReadTargets.push_back(renderTarget->GetID());
    }

    void PassOperations::Write(const FGRenderTarget* renderTarget)
    {
        WriteTargets.push_back(renderTarget->GetID());
    }

    void PassOperations::Write(const FGRenderTargetRef& renderTarget)
    {
        WriteTargets.push_back(renderTarget->GetID());
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
        //GlobalUniformBuffer = new RHIUniformBuffer(EUniformBufferFlags::UniformBuffer_SingleFrame);
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

        ExecutionOrder.clear();
        RenderTargets.clear();
        AllocatedRenderTargets.clear();
        RenderTargetLifetimes.clear();
        PresentTargetID = 0;
        bHasPresentTarget = false;

        // Clear commands from previous frame
        uint32 frameIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
        AllCommands[frameIndex].clear();
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
    }

    void FrameGraph::IntegrateCommands(uint32 frameIndex)
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

    void FrameGraph::Execute()
    {
        uint32 frameIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
        // Execute command before passes
        IntegrateCommands(frameIndex);

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

                IntegrateCommands(frameIndex);
            }

            // After pass execution, release render targets that are no longer needed
            for (const auto& [rtID, lifetime] : RenderTargetLifetimes)
            {
                if (lifetime.second == i) // This is the last use of the render target
                {
                    // Don't release the present target as it might be needed later
                    if (rtID != PresentTargetID)
                    {
                        auto it = AllocatedRenderTargets.find(rtID);
                        if (it != AllocatedRenderTargets.end())
                        {
                            Pool.ReleaseRenderTarget(it->second);
                        }
                    }
                }
            }
        }
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
            registerRequest->CreateUniformBuffer();
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

    void FrameGraph::SetPresentTarget(const FGRenderTarget* renderTarget)
    {
        TAssertf(renderTarget->GetID() != 0xFFFFFFFF, "Present target is not registered yet.");
        PresentTargetID = renderTarget->GetID();
        bHasPresentTarget = true;
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
        if (!bHasPresentTarget)
        {
            return;
        }

        // Mark all current passes as culled initially
        for (const auto& pass : Passes | std::views::values)
        {
            pass->bCulled = true;
        }

        // Use a queue to traverse from present target backwards
        TQueue<uint32> targetQueue;
        THashSet<uint32> neededTargets;

        targetQueue.push(PresentTargetID);
        neededTargets.insert(PresentTargetID);

        // Backwards traversal to find needed passes
        while (!targetQueue.empty())
        {
            uint32 currentTarget = targetQueue.front();
            targetQueue.pop();

            // Find passes that write to this target
            for (auto& pass : Passes)
            {
                if (!CurrentFramePasses.contains(pass.first))
                {
                    continue;
                }
                const auto& writeTargets = pass.second->Operations.GetWriteTargets();
                if (std::ranges::find(writeTargets, currentTarget) != writeTargets.end())
                {
                    pass.second->bCulled = false;

                    // Add read targets to the queue
                    for (uint32 readTarget : pass.second->Operations.GetReadTargets())
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

            for (uint32 readTarget : passX.second->Operations.GetReadTargets())
            {
                for (auto& passY : Passes)
                {
                    if (passX.first == passY.first || passY.second->bCulled) continue;

                    const auto& writeTargets = passY.second->Operations.GetWriteTargets();
                    if (std::ranges::find(writeTargets, readTarget) != writeTargets.end())
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
            for (uint32 writeTarget : Passes[current]->Operations.GetWriteTargets())
            {
                for (auto& pass : Passes)
                {
                    if (pass.second->bCulled) continue;
                    const auto& readTargets = pass.second->Operations.GetReadTargets();
                    if (std::ranges::find(readTargets, writeTarget) != readTargets.end())
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
            for (uint32 rtID : pass->Operations.GetReadTargets())
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
            for (uint32 rtID : pass->Operations.GetWriteTargets())
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
}
