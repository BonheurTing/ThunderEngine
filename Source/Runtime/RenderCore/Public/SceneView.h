#pragma once
#include "Container.h"
#include "Platform.h"

namespace Thunder
{
    enum class EViewType : uint8
    {
        MainView = 0,
        ShadowView = 1,
        Num
    };
    
    // The unit of culling / render context, including temporary status and cache
    class RENDERCORE_API SceneView
    {
    public:
        SceneView(class FrameGraph* owner, EViewType type) : OwnerFrameGraph(owner), ViewType(type) {}

        TArray<class PrimitiveSceneProxy*>& GetVisibleSceneProxies();

    private:
        void CullSceneProxies();
        bool FrustumCull(PrimitiveSceneProxy* sceneProxy) { return true; }
        bool IsCulled() const;

    public:
        
        // camera info
        //FViewMatrices ViewMatrices;
        //FIntRect ViewRect;
        //TVector3d ViewLocation;
        //FRotator ViewRotation;
        //float FOV;

        // render info

        
    private:
        FrameGraph* OwnerFrameGraph;
        EViewType ViewType = EViewType::Num;

        std::atomic_uint32_t CurrentFrameCulled = 0;
        TArray<PrimitiveSceneProxy*> VisibleSceneProxies;
    };

}
