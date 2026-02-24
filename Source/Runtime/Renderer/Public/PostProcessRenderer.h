#pragma once
#include "NameHandle.h"
#include "Vector.h"

namespace Thunder
{
    class FrameGraph;
    struct FrameGraphPass;
    class PassOperations;

    class PostProcessManager
    {
    public:
        PostProcessManager(FrameGraph* frameGraph)
            : Owner(frameGraph)
        {
        }
        void Render(const class FGRenderTarget* inputRT);
    private:
        float Parameter0 = 0.f;
        TVector4f Parameter1;
        TVector4f Parameter2;
        int Parameter3 = 0;

        FrameGraph* Owner = nullptr;
    };
}

