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
        virtual ~MeshPassProcessor() = default;

        RENDERCORE_API virtual bool AddMeshBatch(class FRenderContext* context, const class MeshBatch* batch, EMeshPass meshPassType);
        RENDERCORE_API virtual bool Process(FRenderContext* context, const MeshBatch* batch, EMeshPass meshPassType);
    };
	using MeshPassProcessorRef = TRefCountPtr<MeshPassProcessor>;
}

