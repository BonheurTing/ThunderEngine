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

        void EndRenderFrame()
        {
            if (mFrameGraph)
            {
                mFrameGraph->ClearRenderTargetPool();
            }
        }

        void RegisterSceneInfo(PrimitiveSceneInfo* sceneInfo) const { mFrameGraph->RegisterSceneInfo(sceneInfo); }
        void UnregisterSceneInfo(PrimitiveSceneInfo* sceneInfo) const { mFrameGraph->UnregisterSceneInfo(sceneInfo); }

        FrameGraph* GetFrameGraph() const { return mFrameGraph; }

    protected:
        TRefCountPtr<FrameGraph> mFrameGraph;
    };
}