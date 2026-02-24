#pragma once

#include "IRenderer.h"

namespace Thunder
{
    class RENDERER_API DeferredRenderer :public IRenderer
    {
    public:
        DeferredRenderer();
        ~DeferredRenderer() override;
        void Tick_RenderThread() override;
        void Setup() override;

    private:
        void UpdateAllPrimitiveSceneInfos();
        void InitViews();

    private:
        class PostProcessManager* PostProcessManager = nullptr;
    };
}