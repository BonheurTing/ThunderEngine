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

    extern RENDERER_API TestRenderer* GTestRenderer;
}