#pragma once

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

        virtual void Cull() {}

        void Execute()
        {
            if (mFrameGraph)
            {
                mFrameGraph->Execute();
            }
        }

        void RegisterSceneProxy(PrimitiveSceneProxy* sceneProxy) const { mFrameGraph->RegisterSceneProxy(sceneProxy); }
        void UnregisterSceneProxy(PrimitiveSceneProxy* sceneProxy) const { mFrameGraph->UnregisterSceneProxy(sceneProxy); }

        FrameGraph* GetFrameGraph() const { return mFrameGraph; }

    protected:
        TRefCountPtr<FrameGraph> mFrameGraph;
    };
}