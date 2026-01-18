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

        RENDERCORE_API virtual bool AddMeshBatch(class FRenderContext* context, const class MeshBatch* batch, EMeshPass meshPassType);
        RENDERCORE_API virtual bool Process(FRenderContext* context, const MeshBatch* batch, EMeshPass meshPassType);

        static class TRHIGraphicsPipelineState* GetPipelineState(const class FRenderContext* context, EMeshPass meshPassType, const class SubMesh* subMesh, class RenderMaterial* material);
    };
	using MeshPassProcessorRef = TRefCountPtr<MeshPassProcessor>;
}

