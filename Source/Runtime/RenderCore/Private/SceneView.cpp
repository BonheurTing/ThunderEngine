#include "SceneView.h"

#include "FrameGraph.h"
#include "PrimitiveSceneInfo.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "HAL/Event.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
    class TaskDispatcher;

    void SceneView::CullSceneProxies()
    {
        VisibleStaticSceneInfos.clear();
        VisibleDynamicSceneInfos.clear();

        auto& infoSet = OwnerFrameGraph->GetSceneInfos();
        TArray<PrimitiveSceneInfo*> allSceneInfos( infoSet.begin(), infoSet.end() );
        uint32 proxyNum = static_cast<uint32>(allSceneInfos.size());
        if (proxyNum > 0)
        {
            TArray<TArray<PrimitiveSceneInfo*>> LocalVisibleStaticSceneInfos(GSyncWorkers->GetNumThreads());
            TArray<TArray<PrimitiveSceneInfo*>> LocalVisibleDynamicSceneInfos(GSyncWorkers->GetNumThreads());

            const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
            auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
            dispatcher->Promise(static_cast<int>(proxyNum));
            GSyncWorkers->ParallelFor([&allSceneInfos, &LocalVisibleStaticSceneInfos, &LocalVisibleDynamicSceneInfos, dispatcher, proxyNum, this](uint32 bundleBegin, uint32 bundleSize, uint32 threadId)
            {
                for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
                {
                    if (index >= proxyNum)
                    {
                        break;
                    }

                    auto const& sceneInfo = allSceneInfos[index];
                    if (sceneInfo->NeedRenderView(ViewType))
                    {
                        if (FrustumCull(sceneInfo))
                        {
                            if (sceneInfo->IsMeshDrawCacheSupported())
                            {
                                LocalVisibleStaticSceneInfos[threadId].push_back(sceneInfo);
                            }
                            else
                            {
                                LocalVisibleDynamicSceneInfos[threadId].push_back(sceneInfo);
                            }
                        }
                    }

                    dispatcher->Notify();
                }
            }, proxyNum);

            doWorkEvent->Wait();
            FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
            TMemory::Destroy(dispatcher);

            // Composite.
            size_t staticCount = 0;
            size_t dynamicCount = 0;
            for (const auto& proxies : LocalVisibleStaticSceneInfos)
            {
                staticCount += proxies.size();
            }
            for (const auto& proxies : LocalVisibleDynamicSceneInfos)
            {
                dynamicCount += proxies.size();
            }
            VisibleStaticSceneInfos.reserve(staticCount);
            VisibleDynamicSceneInfos.reserve(dynamicCount);
            for (const auto& proxies : LocalVisibleStaticSceneInfos)
            {
                VisibleStaticSceneInfos.insert(VisibleStaticSceneInfos.end(), proxies.begin(), proxies.end());
            }
            for (const auto& proxies : LocalVisibleDynamicSceneInfos)
            {
                VisibleDynamicSceneInfos.insert(VisibleDynamicSceneInfos.end(), proxies.begin(), proxies.end());
            }
        }

        MarkCulled();
    }
}
