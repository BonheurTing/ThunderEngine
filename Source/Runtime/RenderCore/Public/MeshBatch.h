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

        bool operator<(const MeshBatchKey& other) const
        {
            return LodLevel < other.LodLevel;
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
    };

    class MeshBatch
    {
    public:
        RENDERCORE_API MeshBatch(class PrimitiveSceneInfo* sceneInfo) : SceneInfo(sceneInfo) {}
        RENDERCORE_API virtual ~MeshBatch() = default;

        const TArray<MeshBatchElement>& GetElements() const { return Elements; }

    protected:
        PrimitiveSceneInfo* SceneInfo = nullptr;
        TArray<MeshBatchElement> Elements;
    };

    class StaticMeshBatch : public MeshBatch
    {
    public:
        RENDERCORE_API StaticMeshBatch(PrimitiveSceneInfo* sceneInfo, TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& materials);

    protected:
        TArray<SubMesh*> SubMeshes;
        TArray<RenderMaterial*> Materials;
    };
}

template<>
struct std::hash<Thunder::MeshBatchKey>
{
    size_t operator()(const Thunder::MeshBatchKey& value) const noexcept
    {
        return static_cast<Thunder::uint64>(value.LodLevel);
    }
};
