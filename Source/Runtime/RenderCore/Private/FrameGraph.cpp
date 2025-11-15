#pragma optimize("", off) 
#include "FrameGraph.h"
#include <algorithm>
#include "RenderTexture.h"
#include "RenderContext.h"
#include "RHICommand.h"
#include "IRHIModule.h"
#include "SceneView.h"
#include "Concurrent/TaskGraph.h"
#include "Misc/CoreGlabal.h"


namespace Thunder
{
    void PassOperations::read(const FGRenderTarget& renderTarget)
    {
        ReadTargets.push_back(renderTarget.GetID());
    }

    void PassOperations::write(const FGRenderTarget& renderTarget)
    {
        WriteTargets.push_back(renderTarget.GetID());
    }

    FrameGraph::FrameGraph(int contextNum)
    {
        InitializeRenderContexts(contextNum);
        Views.resize(static_cast<size_t>(EViewType::Num));
        for (auto& view : Views)
        {
            view = new (TMemory::Malloc<SceneView>()) SceneView();
        }
    }

    FrameGraph::~FrameGraph()
    {
        Reset();
        for (auto& view : Views)
        {
            TMemory::Destroy(view);
        }
    }

    void FrameGraph::Reset()
    {
        // Release all allocated render targets back to pool
        for (const auto& [rtID, renderTarget] : AllocatedRenderTargets)
        {
            Pool.ReleaseRenderTarget(renderTarget);
        }

        Passes.clear();
        ExecutionOrder.clear();
        RenderTargetDescs.clear();
        AllocatedRenderTargets.clear();
        RenderTargetLifetimes.clear();
        PresentTargetID = 0;
        bHasPresentTarget = false;
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

    void FrameGraph::Execute()
    {
        // Clear commands from previous frame
        int frameIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
        AllCommands[frameIndex].clear();
        for (auto& context : RenderContexts)
        {
            context->ClearCommands();
            context->FreeAllocator();
        }
        MainContext->ClearCommands();
        MainContext->FreeAllocator();

        // Execute passes according to pseudo-code design
        for (size_t i = 0; i < ExecutionOrder.size(); ++i)
        {
            size_t passIndex = ExecutionOrder[i];
            if (passIndex < Passes.size() && !Passes[passIndex]->bCulled)
            {
                auto& pass = Passes[passIndex];

                if (pass->bIsMeshDrawPass)
                {
                    // TODO: Implement mesh draw pass execution with parallel workers
                    // This matches the pseudo-code for parallel primitive processing
                    // For now, we'll skip this as requested - to be implemented later
                }
                else
                {
                    // Execute regular pass with main context
                    pass->ExecuteFunction(MainContext);

                    // Add main context commands to consolidated list
                    const auto& commands = MainContext->GetCommands();
                    
                    AllCommands[frameIndex].insert(AllCommands[frameIndex].end(), commands.begin(), commands.end());
                    MainContext->ClearCommands();
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
    }

    void FrameGraph::AddPass(const String& name, PassOperations&& operations, PassExecutionFunction&& executeFunction, bool bIsMeshDrawPass)
    {
        auto pass = MakeRefCount<PassData>(name, std::move(operations), std::move(executeFunction), bIsMeshDrawPass);
        Passes.push_back(pass);
    }

    void FrameGraph::InitializeRenderContexts(uint32 threadCount)
    {
        if (!MainContext)
        {
            MainContext = new FRenderContext();
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
            RenderContexts.push_back(new FRenderContext());
        }
    }

    void FrameGraph::SetPresentTarget(const FGRenderTarget& renderTarget)
    {
        PresentTargetID = renderTarget.GetID();
        bHasPresentTarget = true;
        RenderTargetDescs[PresentTargetID] = renderTarget.GetDesc();
    }

    void FrameGraph::RegisterRenderTarget(const FGRenderTarget& renderTarget)
    {
        RenderTargetDescs[renderTarget.GetID()] = renderTarget.GetDesc();
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

        // Mark all passes as culled initially
        for (auto& pass : Passes)
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
                const auto& writeTargets = pass->Operations.GetWriteTargets();
                if (std::find(writeTargets.begin(), writeTargets.end(), currentTarget) != writeTargets.end())
                {
                    pass->bCulled = false;

                    // Add read targets to the queue
                    for (uint32 readTarget : pass->Operations.GetReadTargets())
                    {
                        if (neededTargets.find(readTarget) == neededTargets.end())
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

        TArray<bool> visited(Passes.size(), false);
        TArray<int> inDegree(Passes.size(), 0);

        // Calculate in-degrees
        for (size_t i = 0; i < Passes.size(); ++i)
        {
            if (Passes[i]->bCulled) continue;

            for (uint32 readTarget : Passes[i]->Operations.GetReadTargets())
            {
                for (size_t j = 0; j < Passes.size(); ++j)
                {
                    if (i == j || Passes[j]->bCulled) continue;

                    const auto& writeTargets = Passes[j]->Operations.GetWriteTargets();
                    if (std::find(writeTargets.begin(), writeTargets.end(), readTarget) != writeTargets.end())
                    {
                        inDegree[i]++;
                    }
                }
            }
        }

        // Kahn's algorithm
        TQueue<size_t> queue;
        for (size_t i = 0; i < Passes.size(); ++i)
        {
            if (!Passes[i]->bCulled && inDegree[i] == 0)
            {
                queue.push(i);
            }
        }

        while (!queue.empty())
        {
            size_t current = queue.front();
            queue.pop();
            ExecutionOrder.push_back(current);

            // Update in-degrees for dependent passes
            for (uint32 writeTarget : Passes[current]->Operations.GetWriteTargets())
            {
                for (size_t i = 0; i < Passes.size(); ++i)
                {
                    if (Passes[i]->bCulled) continue;

                    const auto& readTargets = Passes[i]->Operations.GetReadTargets();
                    if (std::find(readTargets.begin(), readTargets.end(), writeTarget) != readTargets.end())
                    {
                        inDegree[i]--;
                        if (inDegree[i] == 0)
                        {
                            queue.push(i);
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
            size_t passIndex = ExecutionOrder[execIndex];
            const auto& pass = Passes[passIndex];

            // Update lifetimes for read targets
            for (uint32 rtID : pass->Operations.GetReadTargets())
            {
                if (RenderTargetLifetimes.find(rtID) == RenderTargetLifetimes.end())
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
                if (RenderTargetLifetimes.find(rtID) == RenderTargetLifetimes.end())
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
            sortedLifetimes.push_back({rtID, lifetime});
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
            if (RenderTargetDescs.find(rtID) == RenderTargetDescs.end())
                continue;

            const auto& desc = RenderTargetDescs[rtID];
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
                        const auto& availableDesc = RenderTargetDescs[availableRtID];

                        // Check if specs are compatible for reuse
                        if (availableDesc == desc)
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
                allocatedTarget = Pool.AcquireRenderTarget(desc);
            }

            AllocatedRenderTargets[rtID] = allocatedTarget;

            // Schedule this target to be available after its lifetime ends
            availableTargets.push_back({lifetime.second, rtID});

            // Keep availableTargets sorted by end time
            std::sort(availableTargets.begin(), availableTargets.end());
        }
    }
}
