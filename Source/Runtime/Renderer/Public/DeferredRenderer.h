#pragma once

#include "IRenderer.h"

namespace Thunder
{
    class RENDERER_API DeferredRenderer :public IRenderer
    {
    public:
        DeferredRenderer();
        ~DeferredRenderer() override = default;
        void Tick_RenderThread() override;
        void Setup() override;

    private:
        void UpdateAllPrimitiveSceneInfos();
    private:
        void InitViews();
    };
}