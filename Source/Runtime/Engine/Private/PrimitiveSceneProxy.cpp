#pragma optimize("", off)
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

    void PrimitiveSceneProxy::UpdateTransform(const TMatrix44f& transform) const
    {
        // Check in render thread
        if (SceneInfo == nullptr) [[unlikely]]
        {
            return;
        }
        SceneInfo->SetTransform(transform);
    }

    StaticMeshSceneProxy::StaticMeshSceneProxy(StaticMeshComponent* inComponent, const TMatrix44f& inTransform)
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
        SceneInfo = new (TMemory::Malloc<StaticMeshSceneInfo>()) StaticMeshSceneInfo{ std::move(subMeshes), std::move(renderMaterials), inTransform };
    }
}
