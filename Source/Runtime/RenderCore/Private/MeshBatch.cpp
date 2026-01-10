
#include "MeshBatch.h"


namespace Thunder
{
    StaticMeshBatch::StaticMeshBatch(PrimitiveSceneInfo* sceneInfo, TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& materials)
        : MeshBatch{ sceneInfo }, SubMeshes(subMeshes), Materials(materials)
    {
        TAssertf(SubMeshes.size() == Materials.size(), "SubMesh count does not match material count.");
        for (uint32 subMeshIndex = 0; subMeshIndex < SubMeshes.size(); ++subMeshIndex)
        {
            Elements.push_back(MeshBatchElement{
                .SubMesh = SubMeshes[subMeshIndex],
                .Material = Materials[subMeshIndex]
            });
        }
    }
}
