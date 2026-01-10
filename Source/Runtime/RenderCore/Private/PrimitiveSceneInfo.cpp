#include "PrimitiveSceneInfo.h"

#include "IDynamicRHI.h"
#include "RenderMaterial.h"
#include "RHICommand.h"
#include "ShaderModule.h"
#include "Memory/TransientAllocator.h"

namespace Thunder
{
    bool PrimitiveSceneInfo::CacheMeshDrawCommand(FRenderContext* context, EMeshPass meshPassType)
    {
        return true;
    }

    void PrimitiveSceneInfo::DestroyStaticMeshes()
    {
        for (auto& staticMesh : StaticMeshes | std::views::values)
        {
            TMemory::Destroy(staticMesh);
        }
        StaticMeshes.clear();
        for (auto& relevance : StaticMeshRelevances | std::views::values)
        {
            TMemory::Destroy(relevance);
        }
        StaticMeshRelevances.clear();
    }

    void PrimitiveSceneInfo::DestroyStaticMesh(MeshBatchKey key)
    {
        auto staticMeshIt = StaticMeshes.find(key);
        if (staticMeshIt != StaticMeshes.end())
        {
            TMemory::Destroy(staticMeshIt->second);
            StaticMeshes.erase(key);
        }
        auto relevanceIt = StaticMeshRelevances.find(key);
        if (relevanceIt != StaticMeshRelevances.end())
        {
            TMemory::Destroy(relevanceIt->second);
            StaticMeshRelevances.erase(key);
        }
    }

    void PrimitiveSceneInfo::AddStaticMesh(MeshBatchKey const& key, TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& renderMaterials)
    {
        TAssertf(subMeshes.size() == renderMaterials.size(), "SubMeshes size mismatch.");
        StaticMeshes[key] = new (TMemory::Malloc<StaticMeshBatch>())
            StaticMeshBatch{ this, subMeshes, renderMaterials };
        StaticMeshRelevances[key] = new (TMemory::Malloc<StaticMeshBatchRelevance>())
            StaticMeshBatchRelevance{ MeshPassMask{ 0xFFffFFffFFffFFffULL } };
    }

    StaticMeshSceneInfo::StaticMeshSceneInfo(TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& materials)
    {
        AddStaticMesh(MeshBatchKey{ .LodLevel = 0 }, subMeshes, materials);
    }

    StaticMeshSceneInfo::~StaticMeshSceneInfo()
    {
        DestroyStaticMeshes();
    }
}
