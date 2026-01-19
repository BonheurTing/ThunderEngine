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

        MeshDrawCommandInfo const& GetDrawCommandInfo(EMeshPass passType, MeshBatchKey batchKey)
        {
            auto& meshDrawInfo = StaticMeshCommandInfos[passType][batchKey];
            return meshDrawInfo;
        }
        void EmplaceDrawCommandInfo(EMeshPass passType, MeshBatchKey meshBatchKey, uint64 commandIndex)
        {
            StaticMeshCommandInfos[passType][meshBatchKey] = MeshDrawCommandInfo{ commandIndex };
        }

        RENDERCORE_API bool CacheMeshDrawCommand(FRenderContext* context, EMeshPass meshPassType);
        RENDERCORE_API bool IsMeshDrawCacheSupported() const  { return MeshDrawCacheSupported; }

    protected:
        void DestroyStaticMeshes();
        void DestroyStaticMesh(MeshBatchKey key);
        void AddStaticMesh(MeshBatchKey const& key, SubMesh* const& subMesh, RenderMaterial* const& material);

    protected:
        TMap<EMeshPass, TMap<MeshBatchKey, MeshDrawCommandInfo>> StaticMeshCommandInfos; // Pass, batch.
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
