#include "PrimitiveSceneProxy.h"
#include "HAL/Event.h"
#include "Compomemt.h"
#include "FrameGraph.h"
#include "IDynamicRHI.h"
#include "PipelineStateCache.h"
#include "RenderMaterial.h"
#include "RHICommand.h"
#include "ShaderArchive.h"
#include "ShaderModule.h"
#include "Misc/CoreGlabal.h"
#include "Memory/TransientAllocator.h"

namespace Thunder
{
    PrimitiveSceneProxy::PrimitiveSceneProxy(PrimitiveComponent* inComponent)
        : Component(inComponent)
    {
    }

    PrimitiveSceneProxy::~PrimitiveSceneProxy()
    {
        TMemory::Destroy(SceneInfo);
    }

    StaticMeshSceneProxy::StaticMeshSceneProxy(StaticMeshComponent* inComponent)
        : PrimitiveSceneProxy(inComponent)
    {
        const TMap<NameHandle, IMaterial*>& gameMaterials = inComponent->GetMaterials();
        TArray<RenderMaterial*> renderMaterials{};
        renderMaterials.reserve(gameMaterials.size());
        for (const auto& val : gameMaterials | std::views::values)
        {
            renderMaterials.push_back(val->GetMaterialResource());
        }
        TArray<SubMesh*> subMeshes = inComponent->GetMesh()->GetSubMeshes();
        SceneInfo = new (TMemory::Malloc<StaticMeshSceneInfo>()) StaticMeshSceneInfo{ std::move(subMeshes), std::move(renderMaterials) };
    }

    void SceneView::CullSceneProxies()
    {
        VisibleSceneInfos.clear();

        auto& infoSet = OwnerFrameGraph->GetSceneInfos();
        TArray<PrimitiveSceneInfo*> allSceneInfos( infoSet.begin(), infoSet.end() );
        uint32 proxyNum = static_cast<uint32>(allSceneInfos.size());
        if (proxyNum > 0)
        {
            TArray<TArray<PrimitiveSceneInfo*>> LocalVisibleProxies(GSyncWorkers->GetNumThreads());

            const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
            auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
            dispatcher->Promise(static_cast<int>(proxyNum));
            GSyncWorkers->ParallelFor([&allSceneInfos, &LocalVisibleProxies, dispatcher, proxyNum, this](uint32 bundleBegin, uint32 bundleSize, uint32 threadId)
            {
                for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
                {
                    if (index >= proxyNum) break;

                    if (allSceneInfos[index]->NeedRenderView(ViewType))
                    {
                        if (FrustumCull(allSceneInfos[index]))
                        {
                            LocalVisibleProxies[threadId].push_back(allSceneInfos[index]);
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
            VisibleSceneInfos.reserve(totalSize);
            for (const auto& proxies : LocalVisibleProxies) {
                VisibleSceneInfos.insert(VisibleSceneInfos.end(), proxies.begin(), proxies.end());
            }
        }
    }


    TArray<PrimitiveSceneInfo*>& SceneView::GetVisibleSceneInfos()
    {
        //TODO main loop
        if (!IsCulled())
        {
            CullSceneProxies();

            uint32 frameNum = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
            CurrentFrameCulled.store(frameNum, std::memory_order_release);
        }
        return VisibleSceneInfos;
    }
}
