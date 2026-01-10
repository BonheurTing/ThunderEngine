#pragma once
#include "PrimitiveSceneInfo.h"
#include "SceneView.h"

namespace Thunder
{
    struct FRenderContext;
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

    protected:
        PrimitiveComponent* Component = nullptr;
        PrimitiveSceneInfo* SceneInfo = nullptr;
    };

    class StaticMeshSceneProxy : public PrimitiveSceneProxy
    {
    public:
        ENGINE_API StaticMeshSceneProxy(class StaticMeshComponent* inComponent);

    };
}

