#include "IRenderer.h"

#include "Concurrent/TaskScheduler.h"

namespace Thunder
{
    IRenderer::IRenderer()
    {
        mFrameGraph = new FrameGraph(GAsyncWorkers->GetNumThreads());
    }

    IRenderer::~IRenderer()
    {
        delete mFrameGraph;
        mFrameGraph = nullptr;
    }
}
