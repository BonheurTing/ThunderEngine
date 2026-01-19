#pragma once
#include <functional>

#include "Container.h"
#include "MeshPass.h"
#include "Platform.h"


namespace Thunder
{
    class SubMesh;
    class RenderMaterial;

    struct MeshBatchKey
    {
        uint32 LodLevel = 0; // Not in use yet.
        uint32 SubMeshIndex = 0;

        bool operator<(const MeshBatchKey& other) const
        {
            return (LodLevel != other.LodLevel) ? (LodLevel < other.LodLevel) : (SubMeshIndex < other.SubMeshIndex);
        }

        bool operator==(const MeshBatchKey& other) const
        {
            return (LodLevel == other.LodLevel) && (SubMeshIndex == other.SubMeshIndex);
        }
    };
    
    class StaticMeshBatchRelevance
    {
    public:
        MeshPassMask CommandInfosMask;
    };

    struct MeshBatchElement
    {
        SubMesh* SubMesh = nullptr;
        RenderMaterial* Material = nullptr;
        uint32 ElementIndex = 0;
    };

    class MeshBatch
    {
    public:
        RENDERCORE_API MeshBatch(class PrimitiveSceneInfo* sceneInfo, MeshBatchKey const& key) : SceneInfo(sceneInfo), Key(key) {}
        RENDERCORE_API virtual ~MeshBatch() = default;

        const TArray<MeshBatchElement>& GetElements() const { return Elements; }
        PrimitiveSceneInfo* GetSceneInfo() const { return SceneInfo; }
        MeshBatchKey GetKey() const { return Key; }

    protected:
        PrimitiveSceneInfo* SceneInfo = nullptr;
        TArray<MeshBatchElement> Elements;
        MeshBatchKey Key;
    };

    class StaticMeshBatch : public MeshBatch
    {
    public:
        RENDERCORE_API StaticMeshBatch(PrimitiveSceneInfo* sceneInfo, class SubMesh* const& subMesh, RenderMaterial* const& material, uint32 lodLevel = 0);
    };
}

template<>
struct std::hash<Thunder::MeshBatchKey>
{
    size_t operator()(const Thunder::MeshBatchKey& value) const noexcept
    {
        return (static_cast<Thunder::uint64>(value.LodLevel) << 32) + value.SubMeshIndex;
    }
};
