#pragma once
#include "MeshPass.h"
#include "Templates/RefCounting.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
    class MeshPassProcessor : public RefCountedObject
    {
    public:
        RENDERCORE_API MeshPassProcessor();
        ~MeshPassProcessor() override = default;

        RENDERCORE_API virtual bool AddMeshBatch(struct RenderContext* context, const class MeshBatch* batch, EMeshPass meshPassType, bool cacheMeshDrawCommand = false);
        RENDERCORE_API virtual bool Process(RenderContext* context, const MeshBatch* batch, EMeshPass meshPassType, bool cacheMeshDrawCommand = false);
        RENDERCORE_API virtual void FinalizeCommand(RenderContext* context, const MeshBatch* batch, class RHIDrawCommand* command, bool cacheMeshDrawCommand = false);

        static class ShaderCombination* GetShaderCombination(EMeshPass meshPassType, const class RenderMaterial* material);
        static class TRHIGraphicsPipelineState* GetPipelineState(const RenderContext* context, ShaderCombination* shaderCombination, EMeshPass meshPassType, const struct SubMesh* subMesh, RenderMaterial* material);
        static void ApplyShaderBindings(RenderContext* context, RHIDrawCommand* command, ShaderCombination* shader, RenderMaterial* material, class PrimitiveSceneInfo* sceneInfo, EMeshPass meshPassType, bool cacheMeshDrawCommand);
    };
	using MeshPassProcessorRef = TRefCountPtr<MeshPassProcessor>;
}

