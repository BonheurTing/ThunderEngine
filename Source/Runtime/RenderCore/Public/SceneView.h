#pragma once
#include "Assertion.h"
#include "Container.h"
#include "Platform.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
    enum class EViewType : uint8
    {
        MainView = 0,
        ShadowView = 1,
        Num
    };
    
    // The unit of culling / render context, including temporary status and cache
    class SceneView
    {
    public:
        RENDERCORE_API SceneView(class FrameGraph* owner, EViewType type) : OwnerFrameGraph(owner), ViewType(type) {}

        RENDERCORE_API void CullSceneProxies();
        RENDERCORE_API bool FrustumCull(class PrimitiveSceneInfo* sceneInfo) { return true; }

        FORCEINLINE bool IsCulled() const
        {
            return CurrentFrameCulled.load(std::memory_order_acquire) >= GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
        }

        FORCEINLINE void MarkCulled()
        {
            uint32 frameNum = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
            CurrentFrameCulled.store(frameNum, std::memory_order_release);
        }

        FORCEINLINE TArray<PrimitiveSceneInfo*>& GetVisibleDynamicSceneInfos()
        {
            TAssertf(IsCulled(), "Failed to get visible scene infos, view is not culled yet.");
            return VisibleDynamicSceneInfos;
        }

        FORCEINLINE TArray<PrimitiveSceneInfo*>& GetVisibleStaticSceneInfos()
        {
            TAssertf(IsCulled(), "Failed to get visible scene infos, view is not culled yet.");
            return VisibleStaticSceneInfos;
        }

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
        TArray<PrimitiveSceneInfo*> VisibleStaticSceneInfos;
        TArray<PrimitiveSceneInfo*> VisibleDynamicSceneInfos;
    };

}
