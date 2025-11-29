#pragma once
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
        ENGINE_API PrimitiveSceneProxy(const PrimitiveComponent* inComponent);
        virtual ~PrimitiveSceneProxy() = default;

        virtual void GetDynamicMeshElements() = 0;

        void AddDrawCall(FRenderContext* context, EMeshPass meshPassType);
        bool NeedRenderView(EViewType type) { return true; }

    private:
        TArray<RenderMaterial*> RenderMaterials; // inComponent manages its lifetime
    };

    class StaticMeshSceneProxy : public PrimitiveSceneProxy
    {
    public:
        ENGINE_API StaticMeshSceneProxy(class StaticMeshComponent* inComponent);

        void GetDynamicMeshElements() override;
    };
}

