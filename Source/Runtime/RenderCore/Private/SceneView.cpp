#include "SceneView.h"

#include "Misc/CoreGlabal.h"

namespace Thunder
{
    bool SceneView::IsCulled() const
    {
        return  CurrentFrameCulled.load(std::memory_order_acquire) >= GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
    }
}
