#include "PrimitiveSceneInfo.h"
#include "IDynamicRHI.h"
#include "RenderContext.h"
#include "RenderMaterial.h"
#include "RenderMesh.h"
#include "RenderModule.h"
#include "ShaderModule.h"
#include "UniformBuffer.h"
#include "Memory/TransientAllocator.h"

namespace Thunder
{
    PrimitiveSceneInfo::PrimitiveSceneInfo(bool meshDrawCacheSupported) :
            MeshDrawCacheSupported(meshDrawCacheSupported)
    {
        //PrimitiveUniformBuffer = new RHIUniformBuffer(EUniformBufferFlags::UniformBuffer_MultiFrame);
    }

    PrimitiveSceneInfo::~PrimitiveSceneInfo()
    {
        if (PrimitiveUniformBuffer)
        {
            delete PrimitiveUniformBuffer;
            PrimitiveUniformBuffer = nullptr;
        }
    }

    void PrimitiveSceneInfo::CreateUniformBuffer()
    {
        if (!PrimitiveUniformBuffer)
        {
            const auto layout = ShaderModule::GetPrimitiveUniformBufferLayout();
            if (!layout) [[unlikely]]
            {
                TAssertf(false, "Cannot update uniform buffer: UniformBufferLayout \"primitive\" not found.");
                return;
            }

            uint32 const bufferSize = layout->GetTotalSize();
            if (bufferSize == 0) [[unlikely]]
            {
                return;
            }

            // Pack and upload CPU data.
            byte* packedData = static_cast<byte*>(TMemory::Malloc(bufferSize, 16));
            memset(packedData, 0, bufferSize);

            PrimitiveUniformBuffer = RHICreateUniformBuffer(bufferSize, EUniformBufferFlags::UniformBuffer_MultiFrame, packedData);
        }
    }

    void PrimitiveSceneInfo::UpdatePrimitiveUniformBuffer(RenderContext* context) const
    {
        const auto layout = ShaderModule::GetPrimitiveUniformBufferLayout();
        if (!layout) [[unlikely]]
        {
            TAssertf(false, "Cannot update uniform buffer: UniformBufferLayout \"primitive\" not found.");
            return;
        }

        uint32 const bufferSize = layout->GetTotalSize();
        if (bufferSize == 0) [[unlikely]]
        {
            return;
        }

        // Pack and upload CPU data.
        byte* packedData = static_cast<byte*>(TMemory::Malloc(bufferSize, 16));
        memset(packedData, 0, bufferSize);

        SetPrimitiveParameter(layout, "LocalToWorld0", Transform.GetRow(0), packedData);
        SetPrimitiveParameter(layout, "LocalToWorld1", Transform.GetRow(1), packedData);
        SetPrimitiveParameter(layout, "LocalToWorld2", Transform.GetRow(2), packedData);

        RHIUpdateUniformBuffer(context, PrimitiveUniformBuffer, packedData);
        //PrimitiveUniformBuffer->UpdateUB(bufferSize);

        TMemory::Free(packedData);
    }

    bool PrimitiveSceneInfo::CacheMeshDrawCommand(RenderContext* context, EMeshPass meshPassType)
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

    StaticMeshSceneInfo::StaticMeshSceneInfo(TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& materials, const TMatrix44f& inTransform)
        : PrimitiveSceneInfo(true)
    {
        Transform = inTransform;
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
    
    void PrimitiveSceneInfo::SetPrimitiveParameter(const UniformBufferLayout* layout, NameHandle parameterName, TVector4f const& value, byte* packedData)
    {
        // Find member.
        auto const& memberMap = layout->GetMemberMap();
        auto memberIt = memberMap.find(parameterName);
        if (memberIt == memberMap.end()) [[unlikely]]
        {
            TAssertf(false, "Failed to set primitive parameter \"%s\", member not found.", parameterName.c_str());
            return;
        }

        // Check type.
        auto const& memberEntry = memberIt->second;
        if (memberEntry.Type != EUniformBufferMemberType::Float4) [[unlikely]]
        {
            TAssertf(false, "Failed to set primitive parameter \"%s\", type error.", parameterName.c_str());
            return;
        }

        memcpy(packedData + memberEntry.Offset, &value, sizeof(TVector4f));
    }

    void PrimitiveSceneInfo::SetPrimitiveParameter(const UniformBufferLayout* layout, NameHandle parameterName, float value, byte* packedData)
    {
        // Find member.
        auto const& memberMap = layout->GetMemberMap();
        auto memberIt = memberMap.find(parameterName);
        if (memberIt == memberMap.end()) [[unlikely]]
        {
            TAssertf(false, "Failed to set primitive parameter \"%s\", member not found.", parameterName.c_str());
            return;
        }

        // Check type.
        auto const& memberEntry = memberIt->second;
        if (memberEntry.Type != EUniformBufferMemberType::Float) [[unlikely]]
        {
            TAssertf(false, "Failed to set primitive parameter \"%s\", type error.", parameterName.c_str());
            return;
        }

        memcpy(packedData + memberEntry.Offset, &value, sizeof(float));
    }

    void PrimitiveSceneInfo::SetPrimitiveParameter(const UniformBufferLayout* layout, NameHandle parameterName, int value, byte* packedData)
    {
        // Find member.
        auto const& memberMap = layout->GetMemberMap();
        auto memberIt = memberMap.find(parameterName);
        if (memberIt == memberMap.end()) [[unlikely]]
        {
            TAssertf(false, "Failed to set primitive parameter \"%s\", member not found.", parameterName.c_str());
            return;
        }

        // Check type.
        auto const& memberEntry = memberIt->second;
        if (memberEntry.Type != EUniformBufferMemberType::Int) [[unlikely]]
        {
            TAssertf(false, "Failed to set primitive parameter \"%s\", type error.", parameterName.c_str());
            return;
        }

        memcpy(packedData + memberEntry.Offset, &value, sizeof(int));
    }
}
