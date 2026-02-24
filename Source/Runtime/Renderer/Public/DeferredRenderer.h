#pragma once

#include "IRenderer.h"

namespace Thunder
{
    class DeferredRenderer :public IRenderer
    {
    public:
        RENDERER_API DeferredRenderer();
        RENDERER_API ~DeferredRenderer() override;
        RENDERER_API void Tick_RenderThread() override;
        RENDERER_API void Setup() override;

    private:
        RENDERER_API void UpdateAllPrimitiveSceneInfos();
        RENDERER_API void InitViews();

    private:
        class PostProcessManager* PostProcessManager = nullptr;
    };
}