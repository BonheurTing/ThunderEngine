#pragma once
#include "SceneView.h"

namespace Thunder
{
    class PrimitiveComponent;

    class PrimitiveSceneProxy
    {
    public:
        ENGINE_API PrimitiveSceneProxy(const PrimitiveComponent* inComponent);
        virtual ~PrimitiveSceneProxy() = default;

        virtual void GetDynamicMeshElements() = 0;

        void AddDrawCall() {}
        bool NeedRenderView(EViewType type) { return true; }
    };

    class StaticMeshSceneProxy : public PrimitiveSceneProxy
    {
    public:
        ENGINE_API StaticMeshSceneProxy(class StaticMeshComponent* inComponent);

        void GetDynamicMeshElements() override;
    };
}

