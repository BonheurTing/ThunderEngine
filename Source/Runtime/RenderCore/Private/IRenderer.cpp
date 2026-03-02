#include "IRenderer.h"

#include "PrimitiveSceneInfo.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/TheadPool.h"
#include "HAL/Event.h"

namespace Thunder
{
    IRenderer::IRenderer()
    {
        FrameGraph = new class FrameGraph(this);
    }

    IRenderer::~IRenderer()
    {
        delete FrameGraph;
        FrameGraph = nullptr;
    }

    void IRenderer::UpdatePrimitiveUniformBuffer_GameThread(PrimitiveSceneInfo* sceneInfo)
    {
        uint32 const gameThreadIndex = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) % 2;
        PrimitiveUniformBufferUpdateSet[gameThreadIndex].insert(sceneInfo);
    }

    void IRenderer::UpdatePrimitiveUniformBuffer_RenderThread()
    {
        uint32 const renderThreadIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
        TSet<PrimitiveSceneInfo*> PrimitiveUniformBufferCurrentUpdateSet;
        PrimitiveUniformBufferCurrentUpdateSet.swap(PrimitiveUniformBufferUpdateSet[renderThreadIndex]);

        // Get scene infos to update.
        TArray<PrimitiveSceneInfo*> sceneInfos{};
        sceneInfos.reserve(PrimitiveUniformBufferCurrentUpdateSet.size());
        for (auto const& sceneInfo : PrimitiveUniformBufferCurrentUpdateSet)
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
        GSyncWorkers->ParallelFor([this, &sceneInfos, dispatcher, sceneInfoCount](uint32 bundleBegin, uint32 bundleSize)
        {
            auto const& contexts = FrameGraph->GetRenderContexts();
            auto context = contexts[GetContextId()];
            for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
            {
                if (index >= sceneInfoCount)
                {
                    break;
                }
                auto sceneInfo = sceneInfos[index];
                sceneInfo->UpdatePrimitiveUniformBuffer(context);

                dispatcher->Notify();
            }
        }, sceneInfoCount);

        // Wait for task to finish.
        doWorkEvent->Wait();
        FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
        TMemory::Destroy(dispatcher);
    }
}
