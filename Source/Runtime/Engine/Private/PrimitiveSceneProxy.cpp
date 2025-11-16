#include "PrimitiveSceneProxy.h"
#include "HAL/Event.h"
//#include "Concurrent/ConcurrentBase.h"
#include "Compomemt.h"
#include "FrameGraph.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
    PrimitiveSceneProxy::PrimitiveSceneProxy(const PrimitiveComponent* inComponent)
    {
        
    }

    StaticMeshSceneProxy::StaticMeshSceneProxy(StaticMeshComponent* inComponent)
        : PrimitiveSceneProxy(inComponent)
    {
        
    }

    void StaticMeshSceneProxy::GetDynamicMeshElements()
    {

        
    }

    void SceneView::CullSceneProxies()
    {
        VisibleSceneProxies.clear();

        auto& proxySet = OwnerFrameGraph->GetSceneProxies();
        TArray<PrimitiveSceneProxy*> allProxies( proxySet.begin(), proxySet.end() );
        uint32 proxyNum = static_cast<uint32>(allProxies.size());
        if (proxyNum > 0)
        {
            TArray<TArray<PrimitiveSceneProxy*>> LocalVisibleProxies(GSyncWorkers->GetNumThreads());

            const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
            auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
            dispatcher->Promise(static_cast<int>(proxyNum));
            GSyncWorkers->ParallelFor([&allProxies, &LocalVisibleProxies, dispatcher, proxyNum, this](uint32 bundleBegin, uint32 bundleSize, uint32 threadId)
            {
                for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
                {
                    if (index >= proxyNum) break;

                    if (allProxies[index]->NeedRenderView(ViewType))
                    {
                        if (FrustumCull(allProxies[index]))
                        {
                            LocalVisibleProxies[threadId].push_back(allProxies[index]);
                        }
                    }

                    dispatcher->Notify();
                }
            }, proxyNum);

            doWorkEvent->Wait();
            FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
            TMemory::Destroy(dispatcher);

            // integrate
            size_t totalSize = 0;
            for (const auto& proxies : LocalVisibleProxies) {
                totalSize += proxies.size();
            }
            VisibleSceneProxies.reserve(totalSize);
            for (const auto& proxies : LocalVisibleProxies) {
                VisibleSceneProxies.insert(VisibleSceneProxies.end(), proxies.begin(), proxies.end());
            }
        }
    }


    TArray<PrimitiveSceneProxy*>& SceneView::GetVisibleSceneProxies()
    {
        //TODO main loop
        if (!IsCulled())
        {
            CullSceneProxies();

            uint32 frameNum = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
            CurrentFrameCulled.store(frameNum, std::memory_order_release);
        }
        return VisibleSceneProxies;
    }
}
