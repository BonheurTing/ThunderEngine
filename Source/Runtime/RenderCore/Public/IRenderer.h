#pragma once

#include "CoreMinimal.h"
#include "RenderCore.export.h"
#include "FrameGraph.h"

namespace Thunder
{
    class RENDERCORE_API IRenderer
    {
    public:
        IRenderer();
        virtual ~IRenderer();

        virtual void Setup() = 0;

        virtual void Compile()
        {
            if (mFrameGraph)
            {
                mFrameGraph->Compile();
            }
        }

        void Execute()
        {
            if (mFrameGraph)
            {
                mFrameGraph->Execute();
            }
        }

    protected:
        FrameGraph* mFrameGraph;
    };
}