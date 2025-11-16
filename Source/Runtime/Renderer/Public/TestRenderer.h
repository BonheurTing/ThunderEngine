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

    private:
        void Print(const std::string& message);
    };


    class RENDERER_API DeferredShadingRenderer :public IRenderer
    {
    public:
        DeferredShadingRenderer();
        virtual ~DeferredShadingRenderer() = default;
        void Setup() override;
    private:
        void InitViews();
        void Print(const std::string& message);
    };
}