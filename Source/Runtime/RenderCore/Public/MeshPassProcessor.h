#pragma once
#include "MeshPass.h"
#include "Platform.h"
#include "Templates/RefCounting.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
    class MeshPassProcessor : public RefCountedObject
    {
    public:
        RENDERCORE_API MeshPassProcessor();
        ~MeshPassProcessor() override = default;

        RENDERCORE_API virtual bool AddMeshBatch(class FRenderContext* context, const class MeshBatch* batch, EMeshPass meshPassType, bool cacheMeshDrawCommand = false);
        RENDERCORE_API virtual bool Process(FRenderContext* context, const MeshBatch* batch, EMeshPass meshPassType, bool cacheMeshDrawCommand = false);
        RENDERCORE_API virtual void FinalizeCommand(FRenderContext* context, const MeshBatch* batch, class RHIDrawCommand* command, bool cacheMeshDrawCommand = false);

        static class ShaderCombination* GetShaderCombination(EMeshPass meshPassType, const class RenderMaterial* material);
        static class TRHIGraphicsPipelineState* GetPipelineState(const class FRenderContext* context, ShaderCombination* shaderCombination, EMeshPass meshPassType, const class SubMesh* subMesh, class RenderMaterial* material);
        static void ApplyShaderBindings(const FRenderContext* context, RHIDrawCommand* command, class ShaderCombination* shader, RenderMaterial* material, bool cacheMeshDrawCommand);
    };
	using MeshPassProcessorRef = TRefCountPtr<MeshPassProcessor>;
}

