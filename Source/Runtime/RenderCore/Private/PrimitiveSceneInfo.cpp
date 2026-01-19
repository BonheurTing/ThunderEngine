#include "PrimitiveSceneInfo.h"

#include "IDynamicRHI.h"
#include "RenderMaterial.h"
#include "RenderMesh.h"
#include "RenderModule.h"
#include "RHICommand.h"
#include "ShaderModule.h"
#include "Memory/TransientAllocator.h"

namespace Thunder
{
    bool PrimitiveSceneInfo::CacheMeshDrawCommand(FRenderContext* context, EMeshPass meshPassType)
    {
        // Add all static mesh batches.
        auto meshProcessor = RenderModule::GetMeshPassProcessor(meshPassType);
        for (const auto& batchIt : StaticMeshes)
        {
            auto const& batchKey = batchIt.first;
            auto const& batch = batchIt.second;

            // Check pass relevance.
            auto relevanceIt = StaticMeshRelevances.find(batchKey);
            if (relevanceIt == StaticMeshRelevances.end()) [[unlikely]]
            {
                TAssertf(false, "Static mesh batch relevance not found.");
                continue;
            }
            if (!relevanceIt->second->CommandInfosMask.Get(meshPassType))
            {
                continue;
            }

            meshProcessor->AddMeshBatch(context, batch, meshPassType, true);
        }

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

    void PrimitiveSceneInfo::AddStaticMesh(MeshBatchKey const& key, SubMesh* const& subMesh, RenderMaterial* const& material)
    {
        StaticMeshes[key] = new (TMemory::Malloc<StaticMeshBatch>())
            StaticMeshBatch{ this, subMesh, material, 0 };
        StaticMeshRelevances[key] = new (TMemory::Malloc<StaticMeshBatchRelevance>())
            StaticMeshBatchRelevance{ MeshPassMask{ 0xFFffFFffFFffFFffULL } };
    }

    StaticMeshSceneInfo::StaticMeshSceneInfo(TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& materials)
        : PrimitiveSceneInfo(true)
    {
        TAssertf(subMeshes.size() == materials.size(), "SubMeshes size mismatch.");
        for (uint32 subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
        {
            SubMesh* subMesh = subMeshes[subMeshIndex];
            RenderMaterial* material = materials[subMeshIndex];
            AddStaticMesh(MeshBatchKey{ .LodLevel = 0, .SubMeshIndex = subMesh->SubMeshIndex }, subMesh, material);
        }
    }

    StaticMeshSceneInfo::~StaticMeshSceneInfo()
    {
        DestroyStaticMeshes();
    }
}
