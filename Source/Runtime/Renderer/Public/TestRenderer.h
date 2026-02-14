#pragma once

#include "CoreMinimal.h"
#include "Renderer.export.h"
#include "IRenderer.h"

namespace Thunder
{
    class RENDERER_API TestRenderer : public IRenderer
    {
    public:
        TestRenderer();
        virtual ~TestRenderer() = default;

        void Setup() override;
        void Tick_RenderThread() override;

    private:
        void Print(const std::string& message);
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
        void Print(const std::string& message);
    };
}