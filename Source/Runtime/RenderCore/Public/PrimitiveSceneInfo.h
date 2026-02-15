#pragma once
#include "Matrix.h"
#include "MeshBatch.h"
#include "SceneView.h"
#include "MeshPass.h"
#include "MeshDrawCommand.h"
#include "NameHandle.h"

namespace Thunder
{
    class RenderMaterial;
    class RHIUniformBuffer;
    struct RenderContext;

    class PrimitiveSceneInfo
    {
    public:
        RENDERCORE_API PrimitiveSceneInfo(bool meshDrawCacheSupported = false);

        virtual ~PrimitiveSceneInfo();

        void SetTransform(const TMatrix44f& matrix) { Transform = matrix; }

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

        RENDERCORE_API void CreateUniformBuffer();
        RENDERCORE_API void UpdatePrimitiveUniformBuffer(RenderContext* context) const;
        const RHIUniformBuffer* GetPrimitiveUniformBuffer() const { return PrimitiveUniformBuffer.IsValid() ? PrimitiveUniformBuffer.Get() : nullptr; }
        RENDERCORE_API bool CacheMeshDrawCommand(RenderContext* context, EMeshPass meshPassType);
        RENDERCORE_API bool IsMeshDrawCacheSupported() const  { return MeshDrawCacheSupported; }

    protected:
        void DestroyStaticMeshes();
        void DestroyStaticMesh(MeshBatchKey key);
        void AddStaticMesh(MeshBatchKey const& key, SubMesh* const& subMesh, RenderMaterial* const& material);

        static void SetPrimitiveParameter(const class UniformBufferLayout* layout, NameHandle parameterName, TVector4f const& value, byte* packedData);
        static void SetPrimitiveParameter(const UniformBufferLayout* layout, NameHandle parameterName, float value, byte* packedData);
        static void SetPrimitiveParameter(const UniformBufferLayout* layout, NameHandle parameterName, int value, byte* packedData);

    protected:
        TMatrix44f Transform;

        TMap<EMeshPass, TMap<MeshBatchKey, MeshDrawCommandInfo>> StaticMeshCommandInfos; // Pass, batch.
        TMap<MeshBatchKey, StaticMeshBatch*> StaticMeshes;
        TMap<MeshBatchKey, StaticMeshBatchRelevance*> StaticMeshRelevances;
        bool MeshDrawCacheSupported = false;

        TRefCountPtr<RHIUniformBuffer> PrimitiveUniformBuffer;
    };

    class StaticMeshSceneInfo : public PrimitiveSceneInfo
    {
    public:
        RENDERCORE_API StaticMeshSceneInfo(TArray<SubMesh*> const& subMeshes, TArray<RenderMaterial*> const& materials, const TMatrix44f& inTransform);
        ~StaticMeshSceneInfo() override;
    };
}
