#include "IRenderer.h"

namespace Thunder
{
    IRenderer::IRenderer()
        : mFrameGraph(new FrameGraph())
    {
    }

    IRenderer::~IRenderer()
    {
        delete mFrameGraph;
        mFrameGraph = nullptr;
    }
}