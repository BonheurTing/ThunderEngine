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
        RENDERCORE_API PrimitiveSceneInfo() = default;
        virtual ~PrimitiveSceneInfo() = default;

        virtual bool NeedRenderView(EViewType type) { return true; }
        TMap<MeshBatchKey, StaticMeshBatch*> const& GetStaticMeshes() { return StaticMeshes; }

        MeshDrawCommandInfo* GetDrawCommandInfo(EMeshPass passType)
        {
            auto commandInfoIt = StaticMeshCommandInfos.find(static_cast<uint32>(passType));
            if (commandInfoIt != StaticMeshCommandInfos.end())
            {
                return &(commandInfoIt->second);
            }
            return nullptr;
        }

        RENDERCORE_API bool CacheMeshDrawCommand(FRenderContext* context, EMeshPass meshPassType);

    protected:
        void DestroyStaticMeshes();
        void DestroyStaticMesh(MeshBatchKey key);
        void AddStaticMesh(MeshBatchKey const& key, TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& renderMaterials);

    protected:
        TMap<uint32, MeshDrawCommandInfo> StaticMeshCommandInfos;
        TMap<MeshBatchKey, StaticMeshBatch*> StaticMeshes;
        TMap<MeshBatchKey, StaticMeshBatchRelevance*> StaticMeshRelevances;
    };

    class StaticMeshSceneInfo : public PrimitiveSceneInfo
    {
    public:
        RENDERCORE_API StaticMeshSceneInfo(TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& materials);
        ~StaticMeshSceneInfo() override;
    };
}
