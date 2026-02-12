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
        static class TRHIGraphicsPipelineState* GetPipelineState(const class RenderContext* context, ShaderCombination* shaderCombination, EMeshPass meshPassType, const class SubMesh* subMesh, class RenderMaterial* material);
        static void ApplyShaderBindings(const RenderContext* context, RHIDrawCommand* command, class ShaderCombination* shader, RenderMaterial* material, bool cacheMeshDrawCommand);
    };
	using MeshPassProcessorRef = TRefCountPtr<MeshPassProcessor>;
}

