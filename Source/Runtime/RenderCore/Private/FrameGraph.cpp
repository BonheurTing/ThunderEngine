#include "FrameGraph.h"
#include "RenderResource.h"
#include <algorithm>


namespace Thunder
{
    void PassOperations::read(const FGRenderTarget& RenderTarget)
    {
        ReadTargets.push_back(RenderTarget.GetID());
    }

    void PassOperations::write(const FGRenderTarget& RenderTarget)
    {
        WriteTargets.push_back(RenderTarget.GetID());
    }

    void FrameGraph::Setup()
    {
        // Clear previous frame data
        Reset();
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
        for (size_t i = 0; i < ExecutionOrder.size(); ++i)
        {
            size_t passIndex = ExecutionOrder[i];
            if (passIndex < Passes.size() && !Passes[passIndex]->bCulled)
            {
                Passes[passIndex]->ExecuteFunction();

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

    void FrameGraph::AddPass(const String& Name, PassOperations&& Operations, PassExecutionFunction&& ExecuteFunction)
    {
        auto pass = MakeRefCount<PassData>(Name, std::move(Operations), std::move(ExecuteFunction));
        Passes.push_back(pass);
    }

    void FrameGraph::SetPresentTarget(const FGRenderTarget& RenderTarget)
    {
        PresentTargetID = RenderTarget.GetID();
        bHasPresentTarget = true;
        RenderTargetDescs[PresentTargetID] = RenderTarget.GetDesc();
    }

    void FrameGraph::RegisterRenderTarget(const FGRenderTarget& RenderTarget)
    {
        RenderTargetDescs[RenderTarget.GetID()] = RenderTarget.GetDesc();
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

    FrameGraphBuilder::FrameGraphBuilder(FrameGraph* InFrameGraph)
        : FrameGraphPtr(InFrameGraph)
    {
    }

    void FrameGraphBuilder::AddPass(const String& Name, PassOperations&& Operations, PassExecutionFunction&& ExecuteFunction) const
    {
        if (FrameGraphPtr)
        {
            FrameGraphPtr->AddPass(Name, std::move(Operations), std::move(ExecuteFunction));
        }
    }

    void FrameGraphBuilder::Present(const FGRenderTarget& RenderTarget) const
    {
        if (FrameGraphPtr)
        {
            FrameGraphPtr->SetPresentTarget(RenderTarget);
        }
    }
}