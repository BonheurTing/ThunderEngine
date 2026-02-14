#pragma once
#include "Matrix.h"
#include "PrimitiveSceneInfo.h"
#include "SceneView.h"

namespace Thunder
{
    struct RenderContext;
    class PrimitiveComponent;
    class RenderMaterial;
    enum class EMeshPass : uint8;

    class PrimitiveSceneProxy
    {
    public:
        ENGINE_API PrimitiveSceneProxy(PrimitiveComponent* inComponent);
        virtual ~PrimitiveSceneProxy();

        bool NeedRenderView(EViewType type) { return true; }
        PrimitiveSceneInfo* GetSceneInfo() const { return SceneInfo; }
        void UpdateTransform(const TMatrix44f& transform) const;

    protected:
        PrimitiveComponent* Component = nullptr;
        PrimitiveSceneInfo* SceneInfo = nullptr;

        //TUniformBufferRef<FPrimitiveUniformShaderParameters> UniformBuffer;
    };

    class StaticMeshSceneProxy : public PrimitiveSceneProxy
    {
    public:
        ENGINE_API StaticMeshSceneProxy(class StaticMeshComponent* inComponent, const TMatrix44f& inTransform);

    };
}

