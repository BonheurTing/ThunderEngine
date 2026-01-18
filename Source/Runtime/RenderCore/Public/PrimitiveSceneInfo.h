#pragma once
#include "MeshBatch.h"
#include "RenderContext.h"
#include "SceneView.h"
#include "MeshPass.h"
#include "MeshDrawCommand.h"


namespace Thunder
{
    class RenderMaterial;

    class PrimitiveSceneInfo
    {
    public:
        RENDERCORE_API PrimitiveSceneInfo(bool meshDrawCacheSupported = false) : MeshDrawCacheSupported(meshDrawCacheSupported) {}
        virtual ~PrimitiveSceneInfo() = default;

        virtual bool NeedRenderView(EViewType type) { return true; }
        TMap<MeshBatchKey, StaticMeshBatch*> const& GetStaticMeshes() { return StaticMeshes; }

        TMap<uint32, MeshDrawCommandInfo> GetDrawCommandInfo(EMeshPass passType)
        {
            auto& meshDrawInfoMap = StaticMeshCommandInfos[passType][MeshBatchKey{}];
            return meshDrawInfoMap;
        }
        void EmplaceDrawCommandInfo(EMeshPass passType, MeshBatchKey meshBatchKey, uint32 subMeshIndex, uint64 commandIndex)
        {
            StaticMeshCommandInfos[passType][meshBatchKey][subMeshIndex] = MeshDrawCommandInfo{ commandIndex };
        }

        RENDERCORE_API bool CacheMeshDrawCommand(FRenderContext* context, EMeshPass meshPassType);
        RENDERCORE_API bool IsMeshDrawCacheSupported() const  { return MeshDrawCacheSupported; }

    protected:
        void DestroyStaticMeshes();
        void DestroyStaticMesh(MeshBatchKey key);
        void AddStaticMesh(MeshBatchKey const& key, TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& renderMaterials);

    protected:
        TMap<EMeshPass, TMap<MeshBatchKey, TMap<uint32, MeshDrawCommandInfo>>> StaticMeshCommandInfos; // Pass, batch, submesh.
        TMap<MeshBatchKey, StaticMeshBatch*> StaticMeshes;
        TMap<MeshBatchKey, StaticMeshBatchRelevance*> StaticMeshRelevances;
        bool MeshDrawCacheSupported = false;
    };

    class StaticMeshSceneInfo : public PrimitiveSceneInfo
    {
    public:
        RENDERCORE_API StaticMeshSceneInfo(TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& materials);
        ~StaticMeshSceneInfo() override;
    };
}
