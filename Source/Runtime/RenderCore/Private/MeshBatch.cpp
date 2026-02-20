
#include "MeshBatch.h"

#include "RenderMesh.h"


namespace Thunder
{
    StaticMeshBatch::StaticMeshBatch(PrimitiveSceneInfo* sceneInfo, SubMesh* const& subMesh, RenderMaterial* const& material, uint32 lodLevel)
        : MeshBatch{ sceneInfo, MeshBatchKey{ .LodLevel = lodLevel, .SubMeshIndex = subMesh->GetSubMeshIndex() } }
    {
        Elements.push_back(MeshBatchElement{
            .SubMesh = subMesh,
            .Material = material,
            .ElementIndex = 0 // Static mesh batch only has one element.
        });
    }
}
