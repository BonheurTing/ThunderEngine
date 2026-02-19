#pragma once

#include "IRenderer.h"

namespace Thunder
{
    class RENDERER_API TestRenderer : public IRenderer
    {
    public:
        TestRenderer() = default;
        virtual ~TestRenderer() = default;

        void Setup() override;
        void Tick_RenderThread() override;
    };


    class RENDERER_API DeferredShadingRenderer :public IRenderer
    {
    public:
        DeferredShadingRenderer();
        virtual ~DeferredShadingRenderer() = default;
        void Tick_RenderThread() override;
        void Setup() override;

    private:
        void UpdateAllPrimitiveSceneInfos();
    private:
        void InitViews();
    };
}